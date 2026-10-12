"""E2E fixtures: host firmware <-> pty <-> device emulator, checked via native API."""

import asyncio
import os
import pty
import re
import subprocess
import sys
import tty
from collections.abc import AsyncIterator, Callable
from pathlib import Path
from typing import Any

import pytest
import yaml
from aioesphomeapi import APIClient, EntityInfo, EntityState

from rka import Device

E2E_DIR = Path(__file__).parent
ROOT_DIR = E2E_DIR.parent.parent
# keep build artifacts in *.tmp, iCloud does not sync them
BUILD_DIR = ROOT_DIR / "build.tmp"
ESPHOME = Path(sys.executable).parent / "esphome"
ESPHOME_ENV = os.environ | {
    "ESPHOME_DATA_DIR": str(BUILD_DIR / "esphome-e2e"),
    "PYTHONPYCACHEPREFIX": str(BUILD_DIR / "pycache"),
}


_ANSI_RE = re.compile(r"\x1b\[[0-9;]*m")


class _Loader(yaml.SafeLoader):
    pass


# esphome tags are irrelevant for reading substitutions
_Loader.add_multi_constructor("!", lambda loader, suffix, node: None)


def _read_config(config: Path) -> dict[str, Any]:
    with config.open(encoding="utf-8") as stream:
        return yaml.load(stream, Loader=_Loader)


@pytest.fixture(scope="session")
def firmware() -> Callable[[str], Path]:
    """Compile tests/e2e/<name>.yaml for host once per session, return binary path."""
    cache: dict[str, Path] = {}

    def build(name: str) -> Path:
        if name not in cache:
            config = E2E_DIR / f"{name}.yaml"
            subprocess.run([ESPHOME, "compile", config], check=True, env=ESPHOME_ENV)
            node = _read_config(config)["esphome"]["name"]
            data_dir = Path(ESPHOME_ENV["ESPHOME_DATA_DIR"])
            cache[name] = data_dir / "build" / node / ".pioenvs" / node / "program"
        return cache[name]

    return build


class Node:
    """Running host firmware connected to a device emulator."""

    def __init__(
        self,
        client: APIClient,
        device: Device,
        entities: list[EntityInfo],
        log_path: Path,
    ):
        self.client = client
        self.device = device
        self.entities = {e.object_id: e for e in entities}
        self.states: dict[int, EntityState] = {}
        self._log_path = log_path
        self._changed = asyncio.Event()

    def key(self, object_id: str) -> int:
        return self.entities[object_id].key

    def on_state(self, state: EntityState) -> None:
        self.states[state.key] = state
        self._changed.set()

    @property
    def logs(self) -> list[str]:
        """Firmware log lines without colors."""
        text = self._log_path.read_text(errors="replace")
        return _ANSI_RE.sub("", text).splitlines()

    async def wait_log(self, text: str, start: int = 0, timeout: float = 5.0) -> str:
        async with asyncio.timeout(timeout):
            while True:
                for line in self.logs[start:]:
                    if text in line:
                        return line
                await asyncio.sleep(0.05)

    async def wait_state(
        self, object_id: str, match: Callable[[Any], bool], timeout: float = 5.0
    ) -> Any:
        key = self.key(object_id)
        async with asyncio.timeout(timeout):
            while True:
                state = self.states.get(key)
                if state is not None and match(state):
                    return state
                self._changed.clear()
                await self._changed.wait()


@pytest.fixture
def node_factory(firmware, tmp_path) -> Callable[[str, Device], Any]:
    async def start(name: str, device: Device) -> AsyncIterator[Node]:
        subs = _read_config(E2E_DIR / f"{name}.yaml")["substitutions"]
        program = firmware(name)

        master, slave = pty.openpty()
        tty.setraw(slave)
        port = Path(subs["uart_port"])
        port.unlink(missing_ok=True)
        port.symlink_to(os.ttyname(slave))
        device.start(master)

        log_path = tmp_path / f"{name}.log"
        log = log_path.open("w")
        proc = subprocess.Popen([program], stdout=log, stderr=subprocess.STDOUT)
        client = APIClient("127.0.0.1", int(subs["api_port"]), None)
        try:
            async with asyncio.timeout(10):
                while True:
                    try:
                        await client.connect(login=True, log_errors=False)
                        break
                    except Exception:  # pylint: disable=broad-except
                        await asyncio.sleep(0.2)
            entities, _ = await client.list_entities_services()
            node = Node(client, device, entities, log_path)
            client.subscribe_states(node.on_state)
            yield node
        finally:
            await client.disconnect()
            proc.terminate()
            proc.wait(timeout=5)
            device.stop()
            os.close(master)
            os.close(slave)
            port.unlink(missing_ok=True)
            log.close()
            print(log_path.read_text())

    return start
