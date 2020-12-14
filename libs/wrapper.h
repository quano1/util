#pragma once

#include <list>
#include <vector>
#include <map>

#include "log.h"

namespace tll{namespace wrapper {

#ifdef TLL_STL_WRAPPER

template <typename T>
class StdList : private std::list<T>
{
public:
    using container_t = std::list<T>;
    using iterator = typename container_t::iterator;
    using const_iterator = typename container_t::const_iterator;
    using value_type = typename container_t::value_type;
    using reference = typename container_t::reference;
    using const_reference = typename container_t::const_reference;

    // using container_t::list;
    using container_t::operator=;
    using container_t::size;
    using container_t::empty;
    using container_t::front;

    auto begin() noexcept {TLL_GLOGO("%p", this);
        return container_t::begin();
    }
    const auto begin() const noexcept {TLL_GLOGO("%p", this);
        return container_t::begin();
    }
    const auto cbegin() const noexcept {TLL_GLOGO("%p", this);
        return container_t::cbegin();
    }
    auto end() noexcept {TLL_GLOGO("%p", this);
        return container_t::end();
    }
    const auto end() const noexcept {TLL_GLOGO("%p", this);
        return container_t::end();
    }
    const auto cend() const noexcept {TLL_GLOGO("%p", this);
        return container_t::cend();
    }

    auto rbegin() noexcept {TLL_GLOGO("%p", this);
        return container_t::rbegin();
    }
    const auto rbegin() const noexcept {TLL_GLOGO("%p", this);
        return container_t::rbegin();
    }
    const auto crbegin() const noexcept {TLL_GLOGO("%p", this);
        return container_t::crbegin();
    }
    auto rend() noexcept {TLL_GLOGO("%p", this);
        return container_t::rend();
    }
    const auto rend() const noexcept {TLL_GLOGO("%p", this);
        return container_t::rend();
    }
    const auto crend() const noexcept {TLL_GLOGO("%p", this);
        return container_t::crend();
    }

    template <class... Args>
    iterator emplace (const_iterator position, Args&&... args) {TLL_GLOGO("%p", this);
        return container_t::emplace(position, std::forward<Args>(args)...);
    }
    template <class... Args>
    void emplace_back (Args&&... args) {TLL_GLOGO("%p", this);
        container_t::emplace_back(std::forward<Args>(args)...);
    }

    void push_back(const value_type &t) {TLL_GLOGO("%p", this);
        container_t::push_back(t);
    }
    void push_back(T &&t) {TLL_GLOGO("%p", this);
        container_t::push_back(t);
    }
    void pop_back() {TLL_GLOGO("%p", this);
        container_t::pop_back();
    }
    
    void push_front(const value_type &t) {TLL_GLOGO("%p", this);
        container_t::push_front(t);
    }
    void push_front(T &&t) {TLL_GLOGO("%p", this);
        container_t::push_front(t);
    }
    void pop_front() {TLL_GLOGO("%p", this);
        container_t::pop_front();
    }

    iterator erase (iterator position) {TLL_GLOGO("%p", this);
        return container_t::erase(position);
    }
    iterator erase (const_iterator first, const_iterator last) {TLL_GLOGO("%p", this);
        return container_t::erase(first, last);
    }

    iterator insert (iterator position, const value_type& val){TLL_GLOGO("%p", this);
        return container_t::insert(position, val);
    }
    void insert (iterator position, size_t n, const value_type& val) {TLL_GLOGO("%p", this);
        container_t::insert(position, n, val);
    }
    template <class InputIterator>
    void insert (iterator position, InputIterator first, InputIterator last) {TLL_GLOGO("%p", this);
        container_t::insert(position, first, last);
    }
    void remove (const value_type& val) {TLL_GLOGO("%p", this);
        container_t::remove(val);
    }
    template <class Predicate>
    void remove_if (Predicate pred) {TLL_GLOGO("%p", this);
        container_t::remove_if(pred);
    }
    void clear() noexcept {TLL_GLOGO("%p", this);
        container_t::clear();
    }

    StdList () : container_t{}
    {TLL_GLOGO("%p", this);}
    template <class ... Args>
    StdList (Args &&...args) : container_t{std::forward<Args>(args)...}
    {TLL_GLOGO("%p", this);}
    ~StdList () 
    {TLL_GLOGO("%p", this);}
};

template <typename T>
class StdVector : private std::vector<T>
{
public:
    using container_t = std::vector<T>;
    using iterator = typename container_t::iterator;
    using const_iterator = typename container_t::const_iterator;
    using value_type = typename container_t::value_type;
    using reference = typename container_t::reference;
    using const_reference = typename container_t::const_reference;

    // using container_t::vector;
    using container_t::size;
    using container_t::resize;
    using container_t::empty;
    using container_t::data;
    using container_t::reserve;
    using container_t::back;
    // using container_t::operator=;
    // using container_t::at;
    // using container_t::operator[];
    StdVector& operator=(const StdVector& x) {TLL_GLOGO("%p", this);
        container_t::operator=(x);
        return *this;
    }
    StdVector& operator=(StdVector&& x) {TLL_GLOGO("%p", this);
        container_t::operator=(x);
        return *this;
    }
    StdVector& operator=(std::initializer_list<value_type> il) {TLL_GLOGO("%p", this);
        container_t::operator=(il);
        return *this;
    }

    reference at (size_t n) {TLL_GLOGO("%p", this);
        return container_t::at(n);
    }
    const_reference at (size_t n) const {TLL_GLOGO("%p", this);
        return container_t::at(n);
    }

    reference& operator[] (size_t n) {TLL_GLOGO("%p", this);
        return container_t::operator[](n);
    }
    const_reference& operator[] (size_t n) const {TLL_GLOGO("%p", this);
        return container_t::operator[](n);
    }
    auto begin() noexcept {TLL_GLOGO("%p", this);
        return container_t::begin();
    }
    const auto begin() const noexcept {TLL_GLOGO("%p", this);
        return container_t::begin();
    }
    const auto cbegin() const noexcept {TLL_GLOGO("%p", this);
        return container_t::cbegin();
    }
    auto end() noexcept {TLL_GLOGO("%p", this);
        return container_t::end();
    }
    const auto end() const noexcept {TLL_GLOGO("%p", this);
        return container_t::end();
    }
    const auto cend() const noexcept {TLL_GLOGO("%p", this);
        return container_t::cend();
    }

    auto rbegin() noexcept {TLL_GLOGO("%p", this);
        return container_t::rbegin();
    }
    const auto rbegin() const noexcept {TLL_GLOGO("%p", this);
        return container_t::rbegin();
    }
    const auto crbegin() const noexcept {TLL_GLOGO("%p", this);
        return container_t::crbegin();
    }
    auto rend() noexcept {TLL_GLOGO("%p", this);
        return container_t::rend();
    }
    const auto rend() const noexcept {TLL_GLOGO("%p", this);
        return container_t::rend();
    }
    const auto crend() const noexcept {TLL_GLOGO("%p", this);
        return container_t::crend();
    }

    template <class... Args>
    iterator emplace (const_iterator position, Args&&... args) {TLL_GLOGO("%p", this);
        return container_t::emplace(position, std::forward<Args>(args)...);
    }
    template <class... Args>
    void emplace_back (Args&&... args) {TLL_GLOGO("%p", this);
        container_t::emplace_back(std::forward<Args>(args)...);
    }

    void push_back(const value_type &t) {TLL_GLOGO("%p", this);
        container_t::push_back(t);
    }
    void push_back(T &&t) {TLL_GLOGO("%p", this);
        container_t::push_back(t);
    }
    void pop_back() {TLL_GLOGO("%p", this);
        container_t::pop_back();
    }
    
    void push_front(const value_type &t) {TLL_GLOGO("%p", this);
        container_t::push_front(t);
    }
    void push_front(T &&t) {TLL_GLOGO("%p", this);
        container_t::push_front(t);
    }
    void pop_front() {TLL_GLOGO("%p", this);
        container_t::pop_front();
    }

    iterator insert (iterator position, const value_type& val){TLL_GLOGO("%p", this);
        return container_t::insert(position, val);
    }
    void insert (iterator position, size_t n, const value_type& val) {TLL_GLOGO("%p", this);
        container_t::insert(position, n, val);
    }
    template <class InputIterator>
    void insert (iterator position, InputIterator first, InputIterator last) {TLL_GLOGO("%p", this);
        container_t::insert(position, first, last);
    }

    void clear() noexcept {TLL_GLOGO("%p", this);
        container_t::clear();
    }

    iterator erase (iterator position) {TLL_GLOGO("%p", this);
        return container_t::erase(position);
    }
    iterator erase (const_iterator first, const_iterator last) {TLL_GLOGO("%p", this);
        return container_t::erase(first, last);
    }

    StdVector () : container_t{}
    {TLL_GLOGO("%p", this);}
    StdVector (const StdVector& s) : container_t{s} 
    {TLL_GLOGO("%p", this);}
    StdVector (StdVector&& s) : container_t{std::move(s)} 
    {TLL_GLOGO("%p", this);}
    template <class ... Args>
    StdVector (Args &&...args) : container_t{std::forward<Args>(args)...}
    {TLL_GLOGO("%p", this);}
    StdVector (std::initializer_list<value_type> il) : container_t{il}
    {TLL_GLOGO("%p", this);}
    ~StdVector () 
    {TLL_GLOGO("%p", this);}
};

template <>
class StdVector<bool> : public std::vector<bool>
{};

template < class K, class T >
class StdMap : private std::map<K, T, std::less<K>, std::allocator<std::pair<const K,T>>>
{
public:
    using container_t = std::map<K, T, std::less<K>, std::allocator<std::pair<const K,T>>>;
    using iterator = typename container_t::iterator;
    using const_iterator = typename container_t::const_iterator;
    using value_type = typename container_t::value_type;
    using mapped_type = typename container_t::mapped_type;
    using key_type = typename container_t::key_type;

    using container_t::empty;
    using container_t::size;

    mapped_type &at(const key_type& k) {TLL_GLOGO("%p", this);
        return container_t::at(k);
    }

    const mapped_type &at(const key_type& k) const {TLL_GLOGO("%p", this);
        return container_t::at(k);
    }

    template <typename K_>
    auto &operator[] (K_ &&k) {TLL_GLOGO("%p", this);
        return container_t::operator[](std::forward<K_>(k));
    }

    iterator find (const key_type& k) {TLL_GLOGO("%p", this);
        return container_t::find(k);
    }

    const_iterator find (const key_type& k) const {TLL_GLOGO("%p", this);
        return container_t::find(k);
    }

    auto begin() noexcept {TLL_GLOGO("%p", this);
        return container_t::begin();
    }
    const auto begin() const noexcept {TLL_GLOGO("%p", this);
        return container_t::begin();
    }
    const auto cbegin() const noexcept {TLL_GLOGO("%p", this);
        return container_t::cbegin();
    }
    auto end() noexcept {TLL_GLOGO("%p", this);
        return container_t::end();
    }
    const auto end() const noexcept {TLL_GLOGO("%p", this);
        return container_t::end();
    }
    const auto cend() const noexcept {TLL_GLOGO("%p", this);
        return container_t::cend();
    }

    auto rbegin() noexcept {TLL_GLOGO("%p", this);
        return container_t::rbegin();
    }
    const auto rbegin() const noexcept {TLL_GLOGO("%p", this);
        return container_t::rbegin();
    }
    const auto crbegin() const noexcept {TLL_GLOGO("%p", this);
        return container_t::crbegin();
    }
    auto rend() noexcept {TLL_GLOGO("%p", this);
        return container_t::rend();
    }
    const auto rend() const noexcept {TLL_GLOGO("%p", this);
        return container_t::rend();
    }
    const auto crend() const noexcept {TLL_GLOGO("%p", this);
        return container_t::crend();
    }

    template <class... Args>
    void emplace (Args&&... args) {TLL_GLOGO("%p", this);
        container_t::emplace(std::forward<Args>(args)...);
    }

    template <class... Args>
    iterator insert (Args&&... args){TLL_GLOGO("%p", this);
        return container_t::insert(std::forward<Args>(args)...);
    }
    void insert (iterator position, size_t n, const value_type& val) {TLL_GLOGO("%p", this);
        container_t::insert(position, n, val);
    }
    template <class InputIterator>
    void insert (iterator position, InputIterator first, InputIterator last) {TLL_GLOGO("%p", this);
        container_t::insert(position, first, last);
    }
    
    std::pair<iterator,bool> insert (const value_type& val) {TLL_GLOGO("%p", this);
        return container_t::insert(val);
    }

    template <class P>
    std::pair<iterator,bool> insert (P&& val) {TLL_GLOGO("%p", this);
        return container_t::insert(val);
    }

    iterator insert (const_iterator position, const value_type& val) {TLL_GLOGO("%p", this);
        return container_t::insert(position, val);
    }

    template <class P> 
    iterator insert (const_iterator position, P&& val) {TLL_GLOGO("%p", this);
        return container_t::insert(position, val);
    }

    template <class InputIterator>
    void insert (InputIterator first, InputIterator last) {TLL_GLOGO("%p", this);
        container_t::insert(first, last);
    }

    void insert (std::initializer_list<value_type> il) {TLL_GLOGO("%p", this);
        container_t::insert(il);
    }

    template <class ... Args>
    auto erase(Args &&...args) {TLL_GLOGO("%p", this);
        return container_t::erase(std::forward<Args>(args)...);
    }

    void clear() noexcept {TLL_GLOGO("%p", this);
        container_t::clear();
    }

    StdMap () : container_t{}
    {TLL_GLOGO("%p", this);}
    template <class ... Args>
    StdMap (Args &&...args) : container_t{std::forward<Args>(args)...}
    {TLL_GLOGO("%p", this);}
    StdMap (std::initializer_list<value_type> il) : container_t{il}
    {TLL_GLOGO("%p", this);}
    ~StdMap ()
    {TLL_GLOGO("%p", this);}
};

#else

template <typename T> 
using StdList = std::list<T, std::allocator<T>>;
template <typename T> 
using StdVector = std::vector<T, std::allocator<T>>;
template <typename K, typename T> 
using StdMap = std::map<K, T, std::less<K>, std::allocator<std::pair<const K,T>>>;

#endif

}}