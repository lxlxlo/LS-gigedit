/*
    Copyright (c) 2014-2016 Christian Schoenebeck
    
    This file is part of "gigedit" and released under the terms of the
    GNU General Public License version 2.
*/

#ifndef GIGEDIT_SETTINGS
#define GIGEDIT_SETTINGS

#include <typeinfo>
#include <glibmm/object.h>
#include <glibmm/property.h>
#include <vector>

/**
 * Reflects, saves and restores all settings for the gigedit application.
 *
 * This class holds a bunch of custom Property objects which can be accessed
 * as if they were basic data types (i.e. by using assignment operator, etc.).
 * As soon as a property gets modified this way, it will automatically be saved
 * to a local config file.
 */
class Settings : public Glib::Object {
public:
    /**
     * Data types for the individual settings.
     */
    enum RawValueType_t {
        BOOLEAN,
        INTEGER,
        UNKNOWN
    };

    /**
     * All settings are grouped into these settings groups.
     */
    enum Group_t {
        GLOBAL
    };

    /**
     * Extension of regular Glib::Property template class; this one
     * automatically calls Settings::onPropertyChanged() method passing this
     * property object as pointer; and it allows to assign new values to this
     * property by using the regular assignment operator; and requires a
     * "Setting group" to be assigned to the property at construction time.
     */
    template<typename T>
    class Property : public Glib::Property<T> {
    public:
        Property(Settings& object, Group_t group, const Glib::ustring& name)
            : Glib::Property<T>::Property(object, name)
        {
            m_settings = &object;
            m_group = group;
            const RawValueType_t type = rawValueType();
            Glib::Property<T>::get_proxy().signal_changed().connect(
                sigc::bind(
                    sigc::bind(
                        sigc::bind(
                            sigc::mem_fun(m_settings, &Settings::onPropertyChanged),
                            m_group
                        ),
                        type
                    ),
                    this
                )
            );
        }

        Property(Settings& object, Group_t group, const Glib::ustring& name, const T& default_value)
            : Glib::Property<T>::Property(object, name, default_value)
        {
            m_settings = &object;
            m_group = group;
            const RawValueType_t type = rawValueType();
            Glib::Property<T>::get_proxy().signal_changed().connect(
                sigc::bind(
                    sigc::bind(
                        sigc::bind(
                            sigc::mem_fun(m_settings, &Settings::onPropertyChanged),
                            m_group
                        ),
                        type
                    ),
                    this
                )
            );
        }

        Property<T>& operator=(const T value) {
            Glib::Property<T>::set_value(value);
            return *this;
        }

        RawValueType_t rawValueType() const {
            const std::string name = typeid(T).name();
            if (name == "bool" || name == "b") return BOOLEAN;
            if (name == "int" || name == "i") return INTEGER;
            return UNKNOWN;
        }

        Group_t group() const { return m_group; }

    private:
        Settings* m_settings;
        Group_t m_group;
    };

    Property<bool> warnUserOnExtensions; ///< if enabled, the user shall he be warned if he is trying to use a gig format extension that will not work with Gigasampler/GigaStudio
    Property<bool> syncSamplerInstrumentSelection; ///< if enabled, the sampler's current instrument will automatically be switched whenever another instrument was selected in gigedit
    Property<bool> moveRootNoteWithRegionMoved; ///< if enabled, the root note(s) of regions are automatically moving when the user drags a region around at the virtual keyboard

    static Settings* singleton();
    Settings();
    void load();

protected:
    void onPropertyChanged(Glib::PropertyBase* pProperty, RawValueType_t type, Group_t group);

private:
    std::vector<Glib::PropertyBase*> m_boolProps; ///< Pointers to all 'bool' type properties this Setting class manages.
    std::vector<Glib::PropertyBase*> m_intProps; ///< Pointers to all 'int' type properties this Setting class manages.
    bool m_ignoreNotifies;
};

#endif // GIGEDIT_SETTINGS
