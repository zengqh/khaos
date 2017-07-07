#pragma once

namespace Khaos
{
    typedef const char* ClassType;
}


#define KHAOS_DECLARE_RTTI(cls) \
    public: \
        static ClassType classType() \
        { \
            static const char name_[] = #cls; \
            return name_; \
        } \
        virtual ClassType getClassType() const \
        { \
            return classType(); \
        }

#define KHAOS_CLASS_TYPE(T)         T::classType()
#define KHAOS_OBJECT_TYPE(obj)      (obj)->getClassType()
#define KHAOS_OBJECT_IS(obj, T)     ((obj)->getClassType() == T::classType())

#define KHAOS_TYPE_TO_NAME(type)    type
#define KHAOS_CLASS_TO_NAME(T)      KHAOS_TYPE_TO_NAME(KHAOS_CLASS_TYPE(T))
