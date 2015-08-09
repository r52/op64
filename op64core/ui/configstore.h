#pragma once

#include <string>
#include <map>
#include <shared_mutex>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>
#include <unordered_set>

#include "oplog.h"

#if defined(_MSC_VER) && ! defined(__INTEL_COMPILER)
#pragma warning( disable : 4018 )
#endif

typedef std::function<void(void)> ConfigCallback;
typedef int64_t CallbackID;
typedef std::unordered_set<CallbackID> CallbackList;

class ConfigStore
{
public:
    static ConfigStore& getInstance()
    {
        static ConfigStore instance;
        return instance;
    }

    bool getBool(std::string section, std::string key)
    {
        std::shared_lock<std::shared_timed_mutex> lock(ptsem);
        return pt.get(section + "." + key, false);
    }

    uint32_t getInt(std::string section, std::string key)
    {
        std::shared_lock<std::shared_timed_mutex> lock(ptsem);
        return pt.get(section + "." + key, 0);
    }

    std::string getString(std::string section, std::string key)
    {
        std::shared_lock<std::shared_timed_mutex> lock(ptsem);
        return pt.get(section + "." + key, "");
    }

    template <class T>
    void set(std::string section, std::string key, T value)
    {
        {
            std::lock_guard<std::shared_timed_mutex> lock(ptsem);
            pt.put(section + "." + key, value);
        }

        CallbackList& list = _callbackMap[section + "." + key];

        if (list.empty())
        {
            // quick out
            return;
        }

        for (CallbackID id : list)
        {
            if (id < _callbackRegistry.size())
            {
                _callbackRegistry[id]();
            }
            else
            {
                LOG_WARNING(ConfigStore) << "Invalid callback id";
            }
        }
    }

    void addChangeCallback(std::string section, std::string key, CallbackID id)
    {
        if (id < 0 || id >= _callbackRegistry.size())
        {
            LOG_ERROR(ConfigStore) << "Invalid callback id";
            return;
        }

        CallbackList& list = _callbackMap[section + "." + key];
        
        list.insert(id);
    }

    void removeChangeCallback(std::string section, std::string key, CallbackID id)
    {
        if (id < 0 || id >= _callbackRegistry.size())
        {
            LOG_ERROR(ConfigStore) << "invalid callback id";
            return;
        }

        CallbackList& list = _callbackMap[section + "." + key];

        auto search = list.find(id);
        if (search != list.end())
        {
            list.erase(search);
        }
    }

    CallbackID registerCallback(ConfigCallback cb)
    {
        if (cb)
        {
            _callbackRegistry.push_back(cb);
            return _callbackRegistry.size() - 1;
        }

        return -1;
    }

    void loadConfig(void);
    void saveConfig(void);

private:
    ConfigStore(void);
    ~ConfigStore(void);

    void setupDefaults(void);

    // Not implemented
    ConfigStore(const ConfigStore&);
    ConfigStore& operator=(const ConfigStore&);

private:
    std::vector<ConfigCallback> _callbackRegistry;
    std::map<std::string, CallbackList> _callbackMap;

    boost::property_tree::ptree pt;

    // Make this untimed when released
    std::shared_timed_mutex ptsem;

    std::string _configFile = "op64.cfg";
};
