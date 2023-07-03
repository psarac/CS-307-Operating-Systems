#ifndef ALLOCATOR_CPP
#define ALLOCATOR_CPP

#include <iostream>
#include <string>
#include <list>
#include <iterator>
#include <unistd.h>
#include <pthread.h>

using namespace std;

struct Chunk {
    int threadID;
    int size;
    int startIndex;
};

class HeapManager {

    private:
        list<Chunk> heap;
        pthread_mutex_t heapLock = PTHREAD_MUTEX_INITIALIZER;
        void printPriv();
    
    public:
        int initHeap(int size);
        int myMalloc(int ID, int size);
        int myFree(int ID, int index);
        void print();
};

int HeapManager::initHeap(int size) {

    Chunk firstChunk;
    firstChunk.threadID = -1;
    firstChunk.size = size;
    firstChunk.startIndex = 0;

    pthread_mutex_lock(&heapLock);

    heap.push_front(firstChunk);

    //cout << "Memory initialized\n";
    printPriv();

    pthread_mutex_unlock(&heapLock);

    return 1;
}

int HeapManager::myMalloc(int ID, int size) {

    int retVal = -1;

    list<Chunk>::iterator iter;

    pthread_mutex_lock(&heapLock);

    for (iter = heap.begin(); iter != heap.end(); ++iter) {

        if ((*iter).threadID == -1 && (*iter).size >= size) { //such chunk is found

            if ((*iter).size == size) { //chunk is exact size
                (*iter).threadID = ID;
                retVal = (*iter).startIndex;
            } else { //there will be some free space left from the chunk 

                Chunk temp;
                temp.size = size;
                temp.startIndex = (*iter).startIndex;
                temp.threadID = ID;

                (*iter).size -= size;
                (*iter).startIndex += size;

                iter = heap.insert(iter, temp);
                retVal = (*iter).startIndex;
            }

            break;
        }
    }

    if (retVal != -1) {
        cout << "Allocated for thread " << ID << endl;
    } else {
        cout << "Can not allocate, requested size " << size << " for thread " << ID << " is bigger than remaining size\n";
    }

    printPriv();

    pthread_mutex_unlock(&heapLock);

    return retVal;

}

int HeapManager::myFree(int ID, int index) {

    int retVal = -1;

    list<Chunk>::iterator iter;

    pthread_mutex_lock(&heapLock);
    
    for (iter = heap.begin(); iter != heap.end(); ++iter) {

        if ((*iter).threadID == ID && (*iter).startIndex == index) {

            retVal = 1;
            (*iter).threadID = -1;

            list<Chunk>::iterator rightIter = iter;
            ++rightIter;

            if (rightIter != heap.end() && (*rightIter).threadID == -1) {

                (*iter).size += (*rightIter).size;
                heap.erase(rightIter);
            }

            if (iter != heap.begin()) {

                list<Chunk>::iterator leftIter = iter;
                --leftIter;

                if ((*leftIter).threadID == -1) {

                    (*iter).size += (*leftIter).size;
                    (*iter).startIndex = (*leftIter).startIndex;
                    heap.erase(leftIter);
                }
            }



            break;
        }
    }


    if (retVal == 1) {
        cout << "Freed for thread " << ID << endl;
    } else {
        cout << "Can not free, no allocation such that ID: " << ID << " and starting index: " << index << " was found\n";
    }

    printPriv();

    pthread_mutex_unlock(&heapLock);

    return retVal;


}

void HeapManager::print() {

    pthread_mutex_lock(&heapLock);
    printPriv();
    pthread_mutex_unlock(&heapLock);

}

void HeapManager::printPriv() {

    list<Chunk>::iterator iter;
    iter = heap.begin();

    //you might want to change this format, or have a mutex later on
    cout << "[" << (*iter).threadID << "][" << (*iter).size << "][" << (*iter).startIndex << "]";
    iter++;

    for (; iter != heap.end(); ++iter) {

        cout << "---[" << (*iter).threadID << "][" << (*iter).size << "][" << (*iter).startIndex << "]";
    }
    cout << endl;
}



#endif
