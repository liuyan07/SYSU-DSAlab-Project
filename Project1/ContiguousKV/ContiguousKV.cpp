#include "ContiguousKV.h"

ContiguousKV::ContiguousKV(int blockCount, int blockSize)
    :n_(blockCount), s_(blockSize)
{
    memory_ = new Block[n_]{};
    bucketCount_ = 10;
    buckets_ = new Request*[bucketCount_]{};
}

ContiguousKV::~ContiguousKV()
{
    // delete memory_
    delete[] memory_;
    // delete 哈希桶
    for(int i=0; i<bucketCount_; i++){
        // delete Request和BlockNode链
        Request* cur = buckets_[i];
        while(cur != nullptr){
            Request* next = cur->hashNext;
            BlockNode* curNode = cur->head;
            while(curNode != nullptr){
                BlockNode* nextNode = curNode->next;
                delete curNode;
                curNode = nextNode;
            }
            delete cur;
            cur = next;
        }
    }
    delete[] buckets_;
}

int hashID(const std::string& id){
    // id中所有字符的asc2码之和 mod 10
    int sum = 0;
    for(char c : id){
        sum += c;
    }
    return sum % 10;
}

bool ContiguousKV::allocate(const std::string& id, int tokenCount) 
{
    if(lookup(id)!=nullptr || tokenCount<=0){
        return false;
    }
    //是否能获得连续分配的内存
    int blockCount = (tokenCount+s_-1)/s_;
    bool flag = false;
    int index = 0, cnt = 0;
    while(index <= n_-blockCount){
        if(memory_[index+cnt].owner == ""){
            cnt++;
            if(cnt == blockCount){
                flag = true;
                break;
            }
        }else{
            index += cnt+1;
            cnt = 0;
        }
    }

    if(flag){
        //新建Request
        Request* newRequest = new Request{};
        newRequest->id = id;
        newRequest->tokenCount = tokenCount;
        newRequest->head = nullptr;
        newRequest->tail = nullptr;
        newRequest->hashNext = nullptr;
        int hashid = hashID(id);
        Request* cur = buckets_[hashid];
        if(cur == nullptr){
            buckets_[hashid] = newRequest;
        }else{
            while(cur->hashNext != nullptr){
                cur = cur->hashNext;
            }
            cur->hashNext = newRequest;
        }
        //分配所需Block
        BlockNode* last = newRequest->head;
        for(int i=0; i<blockCount; i++){
            if(blockCount-i == 1){
                memory_[index+i].owner = id;
                memory_[index+i].used = tokenCount-(blockCount-1)*s_;
            }else{
                memory_[index+i].owner = id;
                memory_[index+i].used = s_;
            }
            BlockNode* cur = new BlockNode{};
            cur->index = index+i;
            cur->next = nullptr;
            if(i==0){
                newRequest->head = cur;
            }else{
                last->next = cur;
            }
            last = cur;
        }
        newRequest->tail = last;
    }
    return flag;
}

bool ContiguousKV::append(const std::string& id, int newTokenCount) 
{
    if(lookup(id)==nullptr || newTokenCount<=0){
        return false;
    }
    Request* request = (Request*)lookup(id);
    Block& lastBlock = memory_[request->tail->index];
    if(newTokenCount+lastBlock.used <= s_){
        lastBlock.used += newTokenCount;
        request->tokenCount += newTokenCount;
        return true;
    }
    //是否能获得连续分配的内存
    int newBlockTokenCount = newTokenCount-(s_-lastBlock.used);
    int blockCount = (newBlockTokenCount+s_-1)/s_;
    bool flag = false;
    int index = request->tail->index+1, cnt = 0;
    while(index <= n_-blockCount){
        if(memory_[index+cnt].owner == ""){
            cnt++;
            if(cnt == blockCount){
                flag = true;
                break;
            }
        }else{
            return false;
        }
    }

    if(flag){
        lastBlock.used = s_;
        //分配新Block
        BlockNode* last = request->tail;
        for(int i=0; i<blockCount; i++){
            if(blockCount-i == 1){
                memory_[index+i].owner = id;
                memory_[index+i].used = newBlockTokenCount-(blockCount-1)*s_;
            }else{
                memory_[index+i].owner = id;
                memory_[index+i].used = s_;
            }
            BlockNode* cur = new BlockNode{};
            cur->index = index+i;
            cur->next = nullptr;
            last->next = cur;
            last = cur;
        }
        request->tail = last;
        request->tokenCount += newTokenCount;
    }
    return flag;
}

const Request* ContiguousKV::lookup(const std::string& id) const 
{
    int hashid = hashID(id);
    Request* cur = buckets_[hashid];
    while(cur != nullptr && cur->id != id){
        cur = cur->hashNext;
    }
    return cur;
}

bool ContiguousKV::free(const std::string& id) 
{
    int hashid = hashID(id);
    Request* cur = buckets_[hashid];
    Request* last = nullptr;
    while(cur != nullptr && cur->id != id){
        last = cur;
        cur = cur->hashNext;
    }
    if(cur == nullptr){
        return false;
    }
    // delete BlockNode链 和 清空memory
    BlockNode* curNode = cur->head;
    while(curNode != nullptr){
        memory_[curNode->index].owner = "";
        memory_[curNode->index].used = 0;
        BlockNode* tempNode = curNode;
        curNode = curNode->next;
        delete tempNode;
    }
    // delete Request
    if(last == nullptr){
        buckets_[hashid] = cur->hashNext;
    }else{
        last->hashNext = cur->hashNext;
    }
    delete cur;
    return true;
}

void ContiguousKV::displayMemory() const 
{
    for(int i=0; i<n_; i++){
        if(i>0){
            std::cout << " ";
        }
        if(memory_[i].owner == ""){
            std::cout << "-";
        }else{
            std::cout << memory_[i].owner;
        }
    }
}

void ContiguousKV::displayRequest(const std::string& id) const 
{
    const Request* request = lookup(id);
    if(request == nullptr){
        std::cout << "NOT FOUND";
        return ;
    }
    int blockCount = (request->tokenCount+s_-1)/s_;
    std::cout << request->tokenCount << " " << blockCount;
    BlockNode* cur = request->head;
    while(cur != nullptr){
        std::cout << " " << cur->index;
        cur = cur->next;
    }

}