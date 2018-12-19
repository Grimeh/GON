#include "gon.h"

#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>

GonObject::ErrorCallback GonObject::error_callback = GonObject::DefaultErrorCallback;

bool IsWhitespace(char c){
    return c==' '||c=='\n'||c=='\r'||c=='\t';
}

bool IsSymbol(char c){
    return c=='='||c==','||c==':'||c=='{'||c=='}'||c=='['||c==']';
}
bool IsIgnoredSymbol(char c){
    return c=='='||c==','||c==':';
}

std::vector<std::string> Tokenize(std::string data){
    std::vector<std::string> tokens;

    bool inString = false;
    bool inComment = false;
    bool escaped = false;
    std::string current_token = "";
    for(int i = 0; i<data.size(); i++){
        if(!inString && !inComment){
            if(IsSymbol(data[i])){
                if(!current_token.empty()){
                    tokens.push_back(current_token);
                    current_token = "";
                }

                if(!IsIgnoredSymbol(data[i])){
                    current_token += data[i];
                    tokens.push_back(current_token);
                    current_token = "";
                }
                continue;
            }

            if(IsWhitespace(data[i])){
                if(!current_token.empty()){
                    tokens.push_back(current_token);
                    current_token = "";
                }

                continue;
            }

            if(data[i] == '#'){
                if(!current_token.empty()){
                    tokens.push_back(current_token);
                    current_token = "";
                }

                inComment = true;
                continue;
            }
            if(data[i] == '"'){
                if(!current_token.empty()){
                    tokens.push_back(current_token);
                    current_token = "";
                }

                inString = true;
                continue;
            }

            current_token += data[i];
        }

        //TODO: escape sequences
        if(inString){
            if(escaped){
                if(data[i] == 'n'){
                    current_token += '\n';
                } else {
                    current_token += data[i];
                }
                escaped = false;
            } else if(data[i] == '\\'){
                escaped = true;
            } else if(!escaped && data[i] == '"'){
                if(!current_token.empty()){
                    tokens.push_back(current_token);
                    current_token = "";
                }
                inString = false;
                continue;
            } else {
                current_token += data[i];
                continue;
            }
        }

        if(inComment){
            if(data[i] == '\n'){
                inComment = false;
                continue;
            }
        }
    }

    return tokens;
}

struct TokenStream {
    std::vector<std::string> Tokens;
    size_t current;
    bool error;

    TokenStream():current(0),error(false){
    }

    std::string Read(){
        if(current>=Tokens.size()){
            error = true;
            return "!";
        }

        return Tokens[current++];
    }
    std::string Peek(){
        if(current>=Tokens.size()){
            error = true;
            return "!";
        }
        return Tokens[current];
    }
};

GonObject::GonObject(){
    name = "";
    type = Type::Null;
    int_data = 0;
    float_data = 0;
    bool_data = false;
    string_data = "";
}

GonObject GonObject::LoadFromTokens(TokenStream& Tokens){
    GonObject ret;

    if(Tokens.Peek() == "{"){         //read object
        ret.type = Type::Object;

        Tokens.Read(); //consume '{'

        while(Tokens.Peek() != "}"){
            std::string name = Tokens.Read();

            ret.children_array.push_back(LoadFromTokens(Tokens));
            ret.children_map[name] = ret.children_array.size()-1;
            ret.children_array[ret.children_array.size()-1].name = name;

            if(Tokens.error) error_callback("GON ERROR: missing a '}' somewhere");
        }

        Tokens.Read(); //consume '}'

        return ret;
    } else if(Tokens.Peek() == "["){  //read array
        ret.type = Type::Array;

        Tokens.Read(); //consume '['
        while(Tokens.Peek() != "]"){
            ret.children_array.push_back(LoadFromTokens(Tokens));

            if(Tokens.error) error_callback("GON ERROR: missing a ']' somewhere");
        }
        Tokens.Read(); //consume ']'

        return ret;
    } else {                          //read data value
        ret.type = Type::String;
        ret.string_data = Tokens.Read();

        //if string data can be converted to a number, do so
        char* endptr;
        ret.int_data = strtoll(ret.string_data.c_str(), &endptr, 0);
        if(*endptr == 0){
            ret.type = Type::Number;
        }

        ret.float_data = strtod(ret.string_data.c_str(), &endptr);
        if(*endptr == 0){
            ret.type = Type::Number;
        }

        //if string data can be converted to a bool or null, convert
        if(ret.string_data == "null") ret.type = Type::Null;
        if(ret.string_data == "true") {
            ret.type = Type::Bool;
            ret.bool_data = true;
        }
        if(ret.string_data == "false") {
            ret.type = Type::Bool;
            ret.bool_data = false;
        }

        return ret;
    }

    return ret;
}

void GonObject::DefaultErrorCallback(const char* msg) {
    throw msg;
}

GonObject GonObject::Load(std::string filename){
    std::ifstream in(filename.c_str(), std::ios::binary);
    in.seekg (0, std::ios::end);
    size_t length = in.tellg();
    in.seekg (0, std::ios::beg);
    std::string str(length + 2, '\0');
    in.read(&str[1], length);
    str.front() = '{';
    str.back() = '}';

    std::vector<std::string> Tokens = Tokenize(str);

    TokenStream ts;
    ts.current = 0;
    ts.Tokens = Tokens;

    return LoadFromTokens(ts);
}

GonObject GonObject::LoadFromBuffer(std::string buffer){
    std::string str = std::string("{")+buffer+"}";
    std::vector<std::string> Tokens = Tokenize(str);

    TokenStream ts;
    ts.current = 0;
    ts.Tokens = Tokens;

    return LoadFromTokens(ts);
}

//options with error throwing
std::string GonObject::String() const {
    if(type != Type::String && type != Type::Number && type != Type::Bool) error_callback("GSON ERROR: Field is not a string");
    return string_data;
}
int GonObject::Int() const {
    if(type != Type::Number) error_callback("GON ERROR: Field is not a number");
    return static_cast<int>(int_data);
}

uint32_t GonObject::UInt() const {
    if(type != Type::Number) error_callback("GON ERROR: Field is not a number");
    return static_cast<uint32_t>(int_data);
}

double GonObject::Number() const {
    if(type != Type::Number) error_callback("GON ERROR: Field is not a number");
    return float_data;
}
bool GonObject::Bool() const {
    if(type != Type::Bool) error_callback("GON ERROR: Field is not a bool");
    return bool_data;
}

//options with a default value
std::string GonObject::String(const std::string& _default) const {
    if(type != Type::String && type != Type::Number && type != Type::Bool) return _default;
    return string_data;
}
int GonObject::Int(int _default) const {
    if(type != Type::Number) return _default;
    return static_cast<int>(int_data);
}
double GonObject::Number(double _default) const {
    if(type != Type::Number) return _default;
    return float_data;
}
bool GonObject::Bool(bool _default) const {
    if(type != Type::Bool) return _default;
    return bool_data;
}

bool GonObject::Contains(const std::string& child) const{
    if(type != Type::Object) return false;

    const auto iter = children_map.find(child);
    return iter != children_map.end();
}
bool GonObject::Contains(int child) const{
    if(type != Type::Array) return true;

    if(child < 0) return false;
    if(static_cast<size_t>(child) >= children_array.size()) return false;
    return true;
}
bool GonObject::Exists() const{
    return type != Type::Null;
}

const GonObject& GonObject::operator[](const std::string& child) const {
    if(type == Type::Null) return null_gon;
    if(type != Type::Object) error_callback("GON ERROR: Field is not an object");

    const auto iter = children_map.find(child);
    if(iter != children_map.end()){
        return children_array[iter->second];
    }

    return null_gon;
    //error_callback("GON ERROR: Field not found: "+child);
}
const GonObject& GonObject::operator[](int childindex) const {
    if(type != Type::Array) error_callback("GSON ERROR: Field is not an array");
    return children_array[childindex];
}
size_t GonObject::Length() const {
    if(type != Type::Array) error_callback("GSON ERROR: Field is not an array");
    return children_array.size();
}



void GonObject::DebugOut(){
    switch (type) {
    case Type::Null:
        std::cout << name << " is null " << std::endl;
        break;

    case Type::String:
        std::cout << name << " is string \"" << String() << "\"" << std::endl;
        break;

    case Type::Number:
        std::cout << name << " is number " << Int() << std::endl;
        break;

    case Type::Object:
        std::cout << name << " is object {" << std::endl;
        for (auto& i : children_array) {
            i.DebugOut();
        }
        std::cout << "}" << std::endl;
        break;

    case Type::Array:
        std::cout << name << " is array [" << std::endl;
        for (auto& i : children_array) {
            i.DebugOut();
        }
        std::cout << "]" << std::endl;
        break;

    case Type::Bool:
        std::cout << name << " is bool " << Bool() << std::endl;
        break;

    default: break;
    }
}


void GonObject::Save(const std::string& filename){
    std::ofstream outfile(filename);
    for (auto& i : children_array) {
        outfile << i.name+" "+ i.getOutStr()+"\n";
    }
    outfile.close();
}

std::string GonObject::getOutStr(){
    std::string out;

    switch (type) {
    case Type::Null:
        out += "null";
        break;

    // If we're a number or bool, we already have a string representation
    // of our value, thanks to the way we parse the input file
    case Type::String:
    case Type::Number:
    case Type::Bool:
        out += String();
        break;

    case Type::Object:
        out += "{\n";
        for (auto& i : children_array) {
            out += i.name+" "+ i.getOutStr()+"\n";
        }
        out += "}\n";
        break;

    case Type::Array:
        out += "[";
        for (auto& i : children_array) {
            out += i.getOutStr()+" ";
        }
        out += "]\n";
        break;

    default:
        break;
    }
    return out;
}

GonObject GonObject::null_gon;
