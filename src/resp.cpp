#include "../include/mini_redis/resp.h"
#include <charconv>

namespace mini_redis {
    void RespParser::append(std::string_view data) {
        this->_buffer.append(data.data(),data.size());
    }

    bool RespParser::parseLine(size_t &pos, std::string &out_line){
        //在缓冲区找到从pos开始第一次以\r\n结束的位置
        size_t end=_buffer.find("\r\n",pos);
        if(end==std::string::npos){
            return false;
        }
        out_line.assign(_buffer.data()+pos,end-pos);
        //将pos向后移动到第一个不为\r\n的起始位置
        pos=end+2;
        return true;
    }

    bool RespParser::parseInteger(size_t& pos, int64_t& out_value){
        std::string line;
        if(parseLine(pos,line)==false){
            return false;
        }
        int64_t v=0;
        auto [ptr,ec]=std::from_chars(line.data(),line.data()+line.size(),v);
        if(ec!=std::errc{}|| ptr!=line.data()+line.size()){
            return false;
        }
        out_value=v;
        return true;
    }

    bool RespParser::parseSimple(size_t& pos, RespType t, RespValue& out){
        std::string s;
        if(!parseLine(pos,s)){
            return false;
        }
       out.type=t;
       out.bulkString=std::move(s); 
       return true;   
    }

    bool RespParser::parseBulkString(size_t& pos, RespValue& out){
        int64_t len=0;
        //解析数字并将游标放置字符串开头
        if(!parseInteger(pos,len)){
            return false;
        }
        // $-1\r\n，解析为 Null
        if(len==-1){
            out.type=RespType::NIL;
            return true;
        }
        if(len<0){
            return false;
        }
        if(_buffer.size()<pos+static_cast<size_t>(len)+2){
            return false;
        }
        out.type=RespType::BULK_STRING;
        out.bulkString.assign(_buffer.data()+pos,static_cast<size_t>(len));
        pos+=static_cast<size_t>(len);//指向字符串结尾的\r
        if(!(pos+1<_buffer.size()&&_buffer[pos]=='\r'&&_buffer[pos+1]=='\n')){
            return false;
        }
        pos+=2;
        return true;
    }

    bool RespParser::parseArray(size_t& pos, RespValue& out){
        int64_t arraySize=0;
        if(!parseInteger(pos,arraySize)){
            return false;
        }
        if(arraySize==-1){
            out.type=RespType::NIL;
            return true;
        }
        if(arraySize<0){
            return false;
        }
        out.type=RespType::ARRAY;
        out.array.clear();
        out.array.reserve(static_cast<size_t>(arraySize));
        for(int i=0;i<arraySize;i++){
            if(pos>=_buffer.size())return false;
            //判断类型,pos移至标识符下一位
            char prefix=_buffer[pos++];
            RespValue rv;
            bool is_ok=false;
            switch (prefix){
                case '+':{
                    is_ok=parseSimple(pos,RespType::SIMPLE_STRING,rv);
                    break;
                }
                case '-':{
                    is_ok=parseSimple(pos,RespType::ERROR,rv);
                    break;
                }
                case ':':{
                    int64_t v=0;
                    is_ok=parseInteger(pos, v);
                    if(is_ok){
                        rv.type=RespType::INTEGER;
                        rv.bulkString=std::to_string(v);
                    }
                    break;
                }
                case '$':{
                    is_ok=parseBulkString(pos, rv);
                    break;
                }
                case '*':{
                    is_ok=parseArray(pos, rv);
                    break;
                }
                default:
                    return false;
            }
            if(!is_ok)return false;
            out.array.emplace_back(std::move(rv));
        }
        return true;
    }

    std::optional<RespValue> RespParser::tryParseOne(){
        if(_buffer.empty()){
            return std::nullopt;
        }
        size_t pos=0;
        //获取标识符并移动游标
        char prefix=_buffer[pos++];
        RespValue rv;
        bool is_ok=false;
        switch (prefix){
            case '+':{
                    is_ok=parseSimple(pos,RespType::SIMPLE_STRING,rv);
                    break;
            }
            case '-':{
                is_ok=parseSimple(pos,RespType::ERROR,rv);
                break;
            }
            case ':':{
                int64_t v=0;
                is_ok=parseInteger(pos, v);
                if(is_ok){
                    rv.type=RespType::INTEGER;
                    rv.bulkString=std::to_string(v);
                }
                break;
            }
            case '$':{
                is_ok=parseBulkString(pos, rv);
                break;
            }
            case '*':{
                is_ok=parseArray(pos, rv);
                break;
            }
            default:
                return std::nullopt;
        }
        if(!is_ok){
            return std::nullopt;
        }
        _buffer.erase(0,pos);
        return std::make_optional<RespValue>(std::move(rv));
    }

    std::optional<std::pair<RespValue,std::string>> RespParser::tryParseOneWithRaw(){
        if(_buffer.empty()){
            return std::nullopt;
        }
        size_t pos=0;
        //获取标识符并移动游标
        char prefix=_buffer[pos++];
        RespValue rv;
        bool is_ok=false;
        switch (prefix){
            case '+':{
                    is_ok=parseSimple(pos,RespType::SIMPLE_STRING,rv);
                    break;
            }
            case '-':{
                is_ok=parseSimple(pos,RespType::ERROR,rv);
                break;
            }
            case ':':{
                int64_t v=0;
                is_ok=parseInteger(pos, v);
                if(is_ok){
                    rv.type=RespType::INTEGER;
                    rv.bulkString=std::to_string(v);
                }
                break;
            }
            case '$':{
                is_ok=parseBulkString(pos, rv);
                break;
            }
            case '*':{
                is_ok=parseArray(pos, rv);
                break;
            }
            default:
                return std::nullopt;
        }
        if(!is_ok){
            return std::nullopt;
        }
        std::string raw=std::string(_buffer.data(),pos);
        _buffer.erase(0,pos);
        auto pair=std::make_pair(std::move(rv),std::move(raw));
        return std::make_optional<std::pair<RespValue,std::string>>(std::move(pair));    
    }

    std::string respSimpleString(std::string_view str){
        return "+"+std::string(str)+"\r\n";
    }
    std::string respError(std::string_view str){
        return "-"+std::string(str)+"\r\n";
    }
    std::string respBulk(std::string_view str){
        return "$"+std::to_string(str.size())+"\r\n"+std::string(str)+"\r\n";
    }
    std::string respNullBulk(){
        return "$-1\r\n";
    }
    std::string respInteger(int64_t str){
        return ":"+std::to_string(str)+"\r\n";
    }
}