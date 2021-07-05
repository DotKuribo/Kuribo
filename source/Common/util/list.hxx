//! @file list.hxx
//!
//! An entirely decentralized list structure: there is no head, and there is no
//! tail.
//! - Given just one element in the chain, all other elements can be accessed:
//!   for (detail::Link* it = node->next; it != node; it = it->next) {
//!      // Do something with element
//!   }
//!   This is wrapped up in the C++ API:
//!   for (auto* it : kuribo::AllNeighborsIncludingSelf(node))
//!
//! - Elements can be `std::move`d to a new memory location, but cannot be
//!   copied.
//!
//! - C++ API abstracts Link, just provides objects:
//!   struct NetworkPeer_Tag {};
//!   struct AccountSession_Tag {};
//!
//!   class NetUser
//!       : kuribo::DLink<NetUser, NetworkPeer_Tag> /* Embedded List */,
//!         kuribo::DLink<NetUser, AccountSession_Tag> /* Embedded List */,
//!   {
//!   public:
//!       auto getPeers() {
//!           return kuribo::AllNeighborsIncludingSelf<NetworkPeer_Tag> {
//!               *this
//!           };
//!       }
//!
//!       auto getAccountSessions() {
//!           return kuribo::AllNeighborsIncludingSelf<AccountSession_Tag> {
//!               *this
//!           };
//!       }
//!
//!       std::string account_name;
//!   };
//!
//!   MyObject* FindPeer(MyObject& obj, std::string_view name) {
//!       auto it = std::ranges::find_if(obj.getPeers(),
//!           [](const MyObject& o) { return o.name == name; });
//!       if (it == obj.getPeers().end()) {
//!           return &*it;
//!       }
//!
//!       return nullptr;
//!   }
//!

#include <common.h>

namespace kuribo {

namespace detail {

struct Link {
  Link* next = nullptr;
  // Last pointer makes move O(1), since otherwise we'd have to go across the
  // whole chain to update the old address.
  Link* last = nullptr;
};

enum class LinkType {
  Orphan,
  Attached,

  Corrupt // Program error, fatal
};

inline LinkType CyclicClassify(const Link& link) {
  if (link.next != nullptr && link.last != nullptr)
    return LinkType::Attached;

  if (link.next == nullptr && link.last == nullptr)
    return LinkType::Orphan;

  return LinkType::Corrupt;
}

inline bool CyclicIsValid(const Link& link) {
  return CyclicClassify(link) != LinkType::Corrupt;
}

// Set first object
inline void CyclicAppendToOrphan(Link& left, Link& right) {
  KURIBO_ASSERT(CyclicClassify(left) == LinkType::Orphan);
  KURIBO_ASSERT(CyclicClassify(right) == LinkType::Orphan);

  left.next = &right;
  left.last = &right;

  right.last = &left;
  right.next = &left;
}

inline void CyclicAppendToNonOrphan(Link& left, Link& right) {
  KURIBO_ASSERT(CyclicClassify(left) == LinkType::Attached);
  KURIBO_ASSERT(CyclicClassify(right) == LinkType::Orphan);

  Link* head = left.next;
  left.next = &right;

  right.next = head;
  right.last = &left;

  head->last = &right;
}

inline void CyclicAppend(Link& left, Link& right) {
  KURIBO_ASSERT(CyclicIsValid(left));
  KURIBO_ASSERT(CyclicIsValid(right));

  if (CyclicClassify(left) == LinkType::Orphan) {
    CyclicAppendToOrphan(left, right);
    return;
  }

  CyclicAppendToNonOrphan(left, right);
}

inline void CyclicPrepend(Link& left, Link& right) {
  KURIBO_ASSERT(CyclicIsValid(left));
  KURIBO_ASSERT(CyclicIsValid(right));

  if (CyclicClassify(left) == LinkType::Orphan) {
    CyclicAppendToOrphan(left, right);
    return;
  }

  CyclicAppendToNonOrphan(*left.last, right);
}

// Prevent stale pointesr
inline void CylicMove(Link& dest, Link&& source) {
  KURIBO_ASSERT(CyclicIsValid(source));

  dest = source;

  if (dest.last != nullptr) {
    KURIBO_ASSERT(dest.last->next == &source);

    dest.last->next = &dest;
  }

  // Not entirely necessary
  source.last = nullptr;
  source.next = nullptr;
}

} // namespace detail

// Decentralized list
template <typename T, typename Tag = void> struct DLink : private detail::Link {
  // Copying would be ambiguous
  DLink(const DLink&) = delete;

  // Moving is tricky, we need to preserve the chain
  DLink(DLink&& rhs) noexcept { detail::CylicMove(getLink_(), rhs.getLink_()); }

  DLink() {
    getLink_().next = nullptr;
    getLink_().last = nullptr;
  }

  void append(DLink& other) {
    detail::CyclicAppend(getLink_(), other.getLink_());
  }
  // append(T&) relies on implicit cast from T& to DLink&
  void prepend(DLink& other) {
    detail::CyclicPrepend(getLink_(), other.getLink_());
  }
  // prepend(T&) relies on implicit cast from T& to DLink&

  // T* as we want to compare address not value of T
  T* getNextCyclic() { return getNextCyclic_(); }
  const T* getNextCyclic() const {
    return const_cast<T*>(this)->getNextCyclic_();
  }

  T* getLastCyclic() { return getLastCyclic_(); }
  const T* getLastCyclic() const {
    return const_cast<T*>(this)->getLastCyclic_();
  }

private:
  // nullptr -> list not setup, return self
  T* fromSublinkPointer_(detail::Link* low) {
    // Orphan case
    if (low == nullptr) {
      return static_cast<T*>(this);
    }

    // The language only gives us one implicit conversion, let's do them
    // explicitly:
    DLink* cpp = static_cast<DLink*>(low);
    // This may or may not involve a pointer translation
    return static_cast<T*>(cpp);
  }

  T* getNextCyclic_() { return fromSublinkPointer_(getLink_().next); }
  T* getLastCyclic_() { return fromSublinkPointer_(getLink_().last); }

  detail::Link& getLink_() { return static_cast<detail::Link&>(*this); }
  detail::LinkType getLinkType_() { return detail::CyclicClassify(getLink_()); }
  T& getDerived_() { return static_cast<T&>(*this); }
  const T& getDerived_() const { return static_cast<const T&>(*this); }
};

template <typename Tag, typename T> struct AllNeighborsIncludingSelf {
  AllNeighborsIncludingSelf(DLink<T, Tag>& obj) : mBegin(&obj) { mIt = mBegin; }

  struct Sentinel {};
  bool operator==(const Sentinel&) const { return isExhausted_(); }
  bool operator!=(const Sentinel&) const { return !isExhausted_(); }

  AllNeighborsIncludingSelf& begin() { return *this; }
  Sentinel end() const { return {}; }

  DLink<T, Tag>& operator++() {
    if (mExhausted)
      return *mIt;

    auto* next = mIt->getNextCyclic();
    if (next == mBegin) {
      mExhausted = true;
    } else {
      mIt = next;
    }

    return *mIt;
  }
  T operator++(int) const {
    auto copy = *this;
    ++copy;
    return copy;
  }

  DLink<T, Tag>& operator*() { return *mIt; }

private:
  bool isExhausted_() const { return mExhausted; }

  DLink<T, Tag>* mBegin;
  DLink<T, Tag>* mIt = nullptr;
  bool mExhausted = false;
};

} // namespace kuribo