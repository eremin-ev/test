/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <cstring>
#include <iostream>
#include <set>
#include <vector>

namespace {

struct KeyManager final {
public:
    enum class Access {
        RO = 1,
        RW,
    };

    enum class Type {
        T_int = 1,
        //T_long,
        T_float,
        //T_double,
    };

    struct ExternalKeyElement {
        const char *name;
        const void *id;
        Access access;
    };

    struct ExternalKeyData {
        const void *id;
        const void *data;
    };

    struct InternalKey {
        InternalKey(const std::string &name, KeyManager::Access access);
        InternalKey(InternalKey &&other);
        ~InternalKey();

        std::string name;
        int data_size;
        void *data;

        ExternalKeyElement key_element;
        ExternalKeyData key_data;
    };

    KeyManager();
    void dump_keys() const;
    int size() const;
    int check() const;
    void fill(ExternalKeyElement *array, int size) const;
    const InternalKey *find_key_by_name(const std::string &name) const;

    std::set<InternalKey, std::less<>> m_keys;
};

KeyManager::InternalKey::InternalKey(const std::string &name_,
                                     KeyManager::Access access_)
    : name{name_}
    , data_size{4}
    , data{malloc(data_size)}
    , key_element {
        .name = name.c_str(),
        .id = key_element.name,
        .access = access_,
      }
    , key_data {
        .id = name.c_str(),
        .data = data,
      }
{
}

KeyManager::InternalKey::InternalKey(InternalKey &&other)
    : name{std::move(other.name)}
    , data_size{other.data_size}
    , data{other.data}
    , key_element {
        .name = name.c_str(),
        .id = key_element.name,
        .access = other.key_element.access,
      }
    , key_data {
        .id = name.c_str(),
        .data = data,
      }
{
    other.data = nullptr;
}

KeyManager::InternalKey::~InternalKey()
{
    free(data);
}

bool operator<(const KeyManager::InternalKey &lhs, const KeyManager::InternalKey &rhs) { return lhs.name < rhs.name; }
bool operator<(const KeyManager::InternalKey &lhs, const std::string &name) { return lhs.name < name; }
bool operator<(const std::string &name, const KeyManager::InternalKey &rhs) { return name < rhs.name; }

KeyManager::KeyManager()
    : m_keys{}
{
    m_keys.emplace(InternalKey("a-read-only-key", Access::RO));
    m_keys.emplace(InternalKey("a-writable-key", KeyManager::Access::RW));
    m_keys.emplace(InternalKey("another-writable-key", KeyManager::Access::RW));
    m_keys.emplace(InternalKey("one-more-ro-key", KeyManager::Access::RO));

    dump_keys();

    int errors = check();
    if (errors) {
        std::cout << "KeyManager errors " << errors << '\n';
    }
}

void KeyManager::dump_keys() const
{
    for (const auto &k : m_keys) {
        std::cout << "KeyManager " << k.name << " data " << k.data << " id " << k.key_element.id << '\n';
    }
}

int KeyManager::size() const
{
    return m_keys.size();
}

int KeyManager::check() const
{
    int errors = 0;
    for (const auto &k : m_keys) {
        errors += k.data != k.key_data.data;
        errors += k.key_element.id != k.key_data.id;
        errors += k.name.c_str() != k.key_element.name;
    }

    return errors;
}

void KeyManager::fill(ExternalKeyElement *array, int size) const
{
    auto k = m_keys.begin();
    for (int i = 0; i < size && k != m_keys.end(); ++i, ++k) {
        array[i].name = k->key_element.name;
        array[i].id = k->key_element.id;
        array[i].access = k->key_element.access;
    }
}

const KeyManager::InternalKey *KeyManager::find_key_by_name(const std::string &name) const
{
    auto n = m_keys.find(name);
    if (n == m_keys.end()) {
        return nullptr;
    }

    return &(*n);
}

struct KeyClient {
public:
    struct CmpById {
        using is_transparent = void;
        bool operator()(const KeyManager::ExternalKeyElement *lhs,
                        const KeyManager::ExternalKeyElement *rhs) const { return lhs->id < rhs->id; }
        bool operator()(const KeyManager::ExternalKeyElement *lhs, const void *id) const { return lhs->id < id; }
        bool operator()(const void *id, const KeyManager::ExternalKeyElement *rhs) const { return id < rhs->id; }
    };

    struct CmpByName {
        using is_transparent = void;
        bool operator()(const KeyManager::ExternalKeyElement *lhs,
                        const KeyManager::ExternalKeyElement *rhs) const
        { return std::strcmp(lhs->name, rhs->name) < 0; }
        bool operator()(const KeyManager::ExternalKeyElement *lhs, const char *name) const
        { return std::strcmp(lhs->name, name) < 0; }
        bool operator()(const char *name, const KeyManager::ExternalKeyElement *rhs) const
        { return std::strcmp(name, rhs->name) < 0; }
    };

    explicit KeyClient(int size);
    void set_pointers();
    void dump_keys() const;
    const KeyManager::ExternalKeyElement *find_by_id(const void *id) const;
    const KeyManager::ExternalKeyElement *find_by_name(const char *name) const;

    std::set<const KeyManager::ExternalKeyElement *, CmpById> keysById;
    std::set<const KeyManager::ExternalKeyElement *, CmpByName> keysByName;
    std::vector<KeyManager::ExternalKeyElement> clientKeys;
};

KeyClient::KeyClient(int size)
{
    clientKeys.resize(size);
}

void KeyClient::set_pointers()
{
    for (const auto &k : clientKeys) {
        keysById.emplace(&k);
        keysByName.emplace(&k);
    }
}

void KeyClient::dump_keys() const
{
    for (const auto &k : clientKeys) {
        std::cout << "KeyClient " << k.name << " access " << (int)k.access << '\n';
    }
}

const KeyManager::ExternalKeyElement *KeyClient::find_by_id(const void *id) const
{
    const auto k = keysById.find(id);
    if (k == keysById.end()) {
        return nullptr;
    }

    return *k;
}

const KeyManager::ExternalKeyElement *KeyClient::find_by_name(const char *name) const
{
    const auto n = keysByName.find(name);
    if (n == keysByName.end()) {
        return nullptr;
    }

    return *n;
}

static void test_manager_by_name(const KeyManager &keyManager,
                                 const std::string &name)
{
    std::cout << "searching " << name << '\n';
    const KeyManager::InternalKey *key = keyManager.find_key_by_name(name);
    if (key) {
        std::cout << "found " << key->name << " data_size "
                              << key->data_size
                              << " id " << key->key_element.id
                              << '\n';
        std::cout << "---" << '\n';
    }
}

static void test_client_by_id(const KeyClient &keyClient, int idx)
{
    std::cout << "searching " << keyClient.clientKeys[idx].name << " by id " << keyClient.clientKeys[idx].id << '\n';
    const KeyManager::ExternalKeyElement *el = keyClient.find_by_id(keyClient.clientKeys[idx].id);
    if (el) {
        std::cout << "found clientKey[" << idx << "] name " << el->name << " id " << el->id << '\n';
        std::cout << "---" << '\n';
    }
}

static void test_client_by_name(const KeyClient &keyClient, int idx)
{
    std::cout << "searching " << keyClient.clientKeys[idx].name << " by name " << keyClient.clientKeys[idx].id << '\n';
    const KeyManager::ExternalKeyElement *el = keyClient.find_by_name(keyClient.clientKeys[idx].name);
    if (el) {
        std::cout << "found clientKey[" << idx << "] name " << el->name << " id " << el->id << '\n';
        std::cout << "---" << '\n';
    }
}

static int test_key_manager()
{
    KeyManager keyManager;

    int size = keyManager.size();

    KeyClient keyClient(size);

    std::cout << "km size " << size << " array " << keyClient.clientKeys.data() << '\n';

    keyManager.fill(keyClient.clientKeys.data(), size);

    int idx = 1;
    test_manager_by_name(keyManager, keyClient.clientKeys[idx].name);

    keyClient.set_pointers();
    keyClient.dump_keys();

    test_client_by_id(keyClient, size - 1);
    test_client_by_name(keyClient, 0);

    // updateKeyById();

    return 0;
}

} // anonymous namespace

int main()
{
    int errors = 0;

    errors += test_key_manager();

    std::cout << "errors " << errors << '\n';

    return 0;
}
