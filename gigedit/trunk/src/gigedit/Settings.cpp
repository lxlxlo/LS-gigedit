/*
    Copyright (c) 2014-2016 Christian Schoenebeck
    
    This file is part of "gigedit" and released under the terms of the
    GNU General Public License version 2.
*/

#include "Settings.h"
#include <glib.h>
#include "global.h"
#include <glibmm/keyfile.h>
#include <iostream>
#include <stdio.h>
#include <fstream>

#if (GTKMM_MAJOR_VERSION == 2 && GTKMM_MINOR_VERSION < 40) || GTKMM_MAJOR_VERSION < 2
# define HAS_GLIB_KEYFILE_SAVE_TO_FILE 0
#else
# define HAS_GLIB_KEYFILE_SAVE_TO_FILE 1
#endif

static std::string configDir() {
    //printf("configDir '%s'\n", g_get_user_config_dir());
    return g_get_user_config_dir();
}

static std::string dirSep() {
    //printf("sep '%s'\n", G_DIR_SEPARATOR_S);
    return G_DIR_SEPARATOR_S;
}

static std::string configFile() {
    return configDir() + dirSep() + "gigedit.conf";
}

static std::string groupName(Settings::Group_t group) {
    switch (group) {
        case Settings::GLOBAL: return "Global";
        case Settings::MAIN_WINDOW: return "MainWindow";
        case Settings::SCRIPT_EDITOR: return "ScriptEditor";
    }
    return "Global";
}

#if !HAS_GLIB_KEYFILE_SAVE_TO_FILE

static bool saveToFile(Glib::KeyFile* keyfile, std::string filename) {
    Glib::ustring s = keyfile->to_data();
    std::ofstream out;
    out.open(filename.c_str(), std::ios_base::out | std::ios_base::trunc);
    out << s;
    out.close();
    return true;
}

#endif // ! HAS_GLIB_KEYFILE_SAVE_TO_FILE

static Settings* _instance = NULL;
    
Settings* Settings::singleton() {
    if (!_instance) {
        _instance = new Settings;
        _instance->load();
    }
    return _instance;
}

Settings::Settings() : Glib::ObjectBase(typeid(Settings)),
    warnUserOnExtensions(*this, GLOBAL, "warnUserOnExtensions", true),
    syncSamplerInstrumentSelection(*this, GLOBAL, "syncSamplerInstrumentSelection", true),
    moveRootNoteWithRegionMoved(*this, GLOBAL, "moveRootNoteWithRegionMoved", true),
    mainWindowX(*this, MAIN_WINDOW, "x", -1),
    mainWindowY(*this, MAIN_WINDOW, "y", -1),
    mainWindowW(*this, MAIN_WINDOW, "w", -1),
    mainWindowH(*this, MAIN_WINDOW, "h", -1),
    scriptEditorWindowX(*this, SCRIPT_EDITOR, "x", -1),
    scriptEditorWindowY(*this, SCRIPT_EDITOR, "y", -1),
    scriptEditorWindowW(*this, SCRIPT_EDITOR, "w", -1),
    scriptEditorWindowH(*this, SCRIPT_EDITOR, "h", -1),
    m_ignoreNotifies(false)
{
    m_boolProps.push_back(&warnUserOnExtensions);
    m_boolProps.push_back(&syncSamplerInstrumentSelection);
    m_boolProps.push_back(&moveRootNoteWithRegionMoved);
    m_intProps.push_back(&mainWindowX);
    m_intProps.push_back(&mainWindowY);
    m_intProps.push_back(&mainWindowW);
    m_intProps.push_back(&mainWindowH);
    m_intProps.push_back(&scriptEditorWindowX);
    m_intProps.push_back(&scriptEditorWindowY);
    m_intProps.push_back(&scriptEditorWindowW);
    m_intProps.push_back(&scriptEditorWindowH);
}

void Settings::onPropertyChanged(Glib::PropertyBase* pProperty, RawValueType_t type, Group_t group) {
    if (m_ignoreNotifies) return;

    //printf("Settings::onPropertyChanged(%s)\n", pProperty->get_name().c_str());

    Glib::KeyFile file;
    try {
        bool ok = file.load_from_file(configFile());
        if (!ok) {
            std::cerr << "Could not load '" << configFile() << "'\n" << std::flush;
        }
    } catch (...) {
        std::cerr << "Could not load '" << configFile() << "'\n" << std::flush;
    }

    switch (type) {
        case BOOLEAN: {
            Property<bool>* prop = static_cast<Property<bool>*>(pProperty);
            //std::cout << "Saving bool setting '" << prop->get_name() << "'\n" << std::flush;
            file.set_boolean(groupName(prop->group()), prop->get_name(), prop->get_value());
            break;
        }
        case INTEGER: {
            Property<int>* prop = static_cast<Property<int>*>(pProperty);
            //std::cout << "Saving int setting '" << prop->get_name() << "'\n" << std::flush;
            file.set_integer(groupName(prop->group()), prop->get_name(), prop->get_value());
            break;
        }
        case UNKNOWN:
            std::cerr << "BUG: Unknown setting raw type of property '" << pProperty->get_name() << "'\n" << std::flush;
            return;
    }

    try {
#if HAS_GLIB_KEYFILE_SAVE_TO_FILE
        bool ok = file.save_to_file(configFile());
#else
        bool ok = saveToFile(&file, configFile());
#endif
        if (!ok) {
            std::cerr << "Failed saving gigedit config to '" << configFile() << "'\n" << std::flush;
        } else {
            //std::cout <<"gigedit CONFIG SAVED\n";
        }
    } catch (...) {
        std::cerr << "Failed saving gigedit config to '" << configFile() << "'\n" << std::flush;
    }
}

void Settings::load() {
    Glib::KeyFile file;
    try {
        bool ok = file.load_from_file(configFile());
        if (!ok) return;
    } catch (...) {
        std::cerr << "Could not load gigedit config file '" << configFile() << "'\n" << std::flush;
        return;
    }

    // ignore onPropertyChanged() calls during updating the property values below
    m_ignoreNotifies = true;

    for (int i = 0; i < m_boolProps.size(); ++i) {
        Property<bool>* prop = static_cast<Property<bool>*>(m_boolProps[i]);
        try {
            const std::string group = groupName(prop->group());
            if (!file.has_key(group, prop->get_name())) continue;
            const bool value = file.get_boolean(group, prop->get_name());
            prop->set_value(value);
        } catch (...) {
            continue;
        }
    }

    for (int i = 0; i < m_intProps.size(); ++i) {
        Property<int>* prop = static_cast<Property<int>*>(m_intProps[i]);
        try {
            const std::string group = groupName(prop->group());
            if (!file.has_key(group, prop->get_name())) continue;
            const int value = file.get_integer(group, prop->get_name());
            prop->set_value(value);
        } catch (...) {
            continue;
        }
    }

    m_ignoreNotifies = false;
}

