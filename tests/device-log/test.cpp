#include "Log.h"
#include <cassert>
#include <string>
#include <thread>
#include <vector>
#include <iostream>
FakeSerial Serial;
void put(DeviceLog&log,const std::string&s){log.write((const uint8_t*)s.data(),s.size());}
int main(){
DeviceLog log;log.begin();char out[LOG_BUFFER_SIZE];uint64_t end=0,cursor=0;bool dropped;
assert(log.snapshot(out,sizeof(out),end)==0);
put(log,"hello");put(log," world\nnext\n");auto n=log.snapshot(out,sizeof(out),end);
assert(std::string(out,n)=="[42] hello world\n[42] next\n");
assert(log.readSince(cursor,out,4,dropped)==4&&!dropped&&cursor==4);
put(log,std::string(LOG_BUFFER_SIZE*3,'x'));n=log.snapshot(out,sizeof(out),end);
assert(n==LOG_BUFFER_SIZE&&std::string(out,n)==std::string(LOG_BUFFER_SIZE,'x'));
assert(log.readSince(cursor,out,sizeof(out),dropped)==LOG_BUFFER_SIZE&&dropped&&cursor==end);
assert(log.readSince(cursor,out,sizeof(out),dropped)==0&&!dropped);
cursor=end+1;assert(log.readSince(cursor,out,16,dropped)==16&&dropped);
DeviceLog concurrent;concurrent.begin();std::vector<std::thread> writers;
for(int i=0;i<4;i++)writers.emplace_back([&]{for(int j=0;j<100;j++)put(concurrent,"task\n");});
for(auto&t:writers)t.join();n=concurrent.snapshot(out,sizeof(out),end);
std::string result(out,n);size_t lines=0,pos=0;while(pos<result.size()){assert(result.substr(pos,10)=="[42] task\n");pos+=10;lines++;}assert(lines==400);
std::cout<<"DeviceLog: prefix, split writes, overflow, cursors, concurrent writers passed\n";
}
