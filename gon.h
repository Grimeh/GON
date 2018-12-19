//Glaiel Object Notation
//its json, minus the crap!

#pragma once
#include <string>
#include <map>
#include <vector>

struct TokenStream;

class GonObject {
    std::map<std::string, size_t> children_map;
    std::vector<GonObject> children_array;
    std::string string_data;
    int64_t int_data;
    double float_data;
    bool bool_data;

    static GonObject LoadFromTokens(TokenStream& Tokens);
    static void DefaultErrorCallback(const char* msg);

    public:
        enum class Type {
            Null,

            String,
            Number,
            Object,
            Array,
            Bool
        };
        Type type;

        std::string name;

        static GonObject null_gon;

        using ErrorCallback = void(*)(const char*);
        static ErrorCallback error_callback;

    public:
        static GonObject Load(std::string filename);
        static GonObject LoadFromBuffer(std::string buffer);

        GonObject();

        //throw error if accessing wrong type, otherwise return correct type
        std::string String() const;
        int Int() const;
        uint32_t UInt() const;
        double Number() const;
        bool Bool() const;

        //returns a default value if the field doesn't exist or is the wrong type
        std::string String(const std::string& _default) const;
        int Int(int _default) const;
        double Number(double _default) const;
        bool Bool(bool _default) const;

        bool Contains(const std::string& child) const;
        bool Contains(int child) const;
        bool Exists() const; //true if non-null

        //returns null_gon if the field does not exist. 
        const GonObject& operator[](const std::string& child) const;

        //returns self if index is not an array, 
        //all objects can be considered an array of size 1 with themselves as the member
        const GonObject& operator[](int childindex) const;
        size_t Length() const;

        void DebugOut();

        void Save(const std::string& outfilename);
        std::string getOutStr();
};
