#pragma once

#include <cstdint>

struct ipv4_addr {
  uint32_t addr;
};
struct ip_addr_t : ipv4_addr {};

typedef uint32_t u32_t;

inline void ipaddr_aton(const char *ip, ip_addr_t *addr) {
  if (addr) {
    addr->addr = 0;
  }
}

class IPAddress {
 private:
  ip_addr_t ip_{};

  // Access the raw byte array containing the address.  Because this returns a pointer
  // to the internal structure rather than a copy of the address this function should only
  // be used when you know that the usage of the returned uint8_t* will be transient and not
  // stored.
  uint8_t *raw_address() { return reinterpret_cast<uint8_t *>(&v4()); }
  const uint8_t *raw_address() const { return reinterpret_cast<const uint8_t *>(&v4()); }

 public:
  // Constructors
  IPAddress() {}
  IPAddress(const IPAddress &from) : ip_(from.ip_) {}
  IPAddress(const ip_addr_t &addr) : ip_(addr) {}
  // IPAddress(uint8_t first_octet, uint8_t second_octet, uint8_t third_octet, uint8_t fourth_octet);
  // IPAddress(uint32_t address) {this->ip_.addr = addr; }
  // IPAddress(u32_t address) {this->ip_.addr = addr; }
  // IPAddress(int address) { this->ip_.addr = addr; }
  // IPAddress(const uint8_t *address);

  // bool fromString(const char *address);
  // bool fromString(const std::string &address) { return fromString(address.c_str()); }

  // Overloaded cast operator to allow IPAddress objects to be used where a pointer
  // to a four-byte uint8_t array is expected
  operator uint32_t() const { return v4(); }
  operator uint32_t() { return v4(); }
  // operator u32_t() const { return v4(); }
  // operator u32_t() { return v4(); }

  bool isSet() const;
  operator bool() const { return isSet(); }  // <-
  operator bool() { return isSet(); }        // <- both are needed

  // generic IPv4 wrapper to uint32-view like arduino loves to see it
  const u32_t &v4() const { return this->ip_.addr; }  // for raw_address(const)
  u32_t &v4() { return this->ip_.addr; }

  bool operator==(const IPAddress &addr) const { return addr.v4() == this->v4(); }
  bool operator!=(const IPAddress &addr) const { return addr.v4() != this->v4(); }
  bool operator==(uint32_t addr) const { return v4() == addr; }
  // bool operator==(u32_t addr) const { return v4() == addr; }
  bool operator!=(uint32_t addr) const { return !(v4() == addr); }
  // bool operator!=(u32_t addr) const { return !(v4() == addr); }
  bool operator==(const uint8_t *addr) const;

  int operator>>(int n) const { return v4() >> n; }

  // Overloaded index operator to allow getting and setting individual octets of the address
  uint8_t operator[](int index) const { return *(raw_address() + index); }
  uint8_t &operator[](int index) { return *(raw_address() + index); }

  // Overloaded copy operators to allow initialisation of IPAddress objects from other types
  // IPAddress &operator=(const uint8_t *address);
  // IPAddress &operator=(uint32_t address);
  IPAddress &operator=(const IPAddress &) = default;

  // virtual size_t printTo(Print &p) const;
  std::string toString() const {
    std::string s;
    s += std::to_string(this->ip_.addr >> 0x18 & 0xFF);
    s += '.';
    s += std::to_string(this->ip_.addr >> 0x10 & 0xFF);
    s += '.';
    s += std::to_string(this->ip_.addr >> 0x08 & 0xFF);
    s += '.';
    s += std::to_string(this->ip_.addr >> 0x00 & 0xFF);
    return s;
  }

  void clear();

  /*
          check if input string(arg) is a valid IPV4 address or not.
          return true on valid.
          return false on invalid.
  */
  static bool isValid(const std::string &arg);
  static bool isValid(const char *arg);

  friend class EthernetClass;
  friend class UDP;
  friend class Client;
  friend class Server;
  friend class DhcpClass;
  friend class DNSClient;

  /*
         lwIP address compatibility
  */
  IPAddress(const ipv4_addr &fw_addr) { this->ip_.addr = fw_addr.addr; }
  IPAddress(const ipv4_addr *fw_addr) { this->ip_.addr = fw_addr->addr; }

  IPAddress &operator=(const ipv4_addr &fw_addr) {
    this->ip_.addr = fw_addr.addr;
    return *this;
  }
  IPAddress &operator=(const ipv4_addr *fw_addr) {
    this->ip_.addr = fw_addr->addr;
    return *this;
  }

  operator ip_addr_t() const { return ip_; }
  operator const ip_addr_t *() const { return &ip_; }
  operator ip_addr_t *() { return &ip_; }

  bool isV4() const { return true; }
  void setV4() const {}
  // bool isLocal() const { return ip_addr_islinklocal(&ip_); }

  // allow portable code when IPv6 is not enabled
  uint16_t *raw6() { return nullptr; }
  const uint16_t *raw6() const { return nullptr; }
  bool isV6() const { return false; }
  void setV6() const {}

 protected:
  bool fromString4(const char *address);
};

// extern const IPAddress INADDR_ANY;
// extern const IPAddress INADDR_NONE;
