/* graph.hpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#ifndef _GRAPH_H
#define _GRAPH_H

#include <vector>
#include <list>
#include <string>
#include <boost/unordered_map.hpp>

namespace ismt
{

    class HeadNode
    {
    private:
        int index;
    public:
        HeadNode(int i):index(i){}
        ~HeadNode(){}

        std::list<int> neighbors;

        // insertion sort
        void addNeighbor(int val){
            std::list<int>::iterator it = neighbors.begin();
            if(*(neighbors.begin()) > val){ neighbors.insert(neighbors.begin(), val); return; }
            while(it!=neighbors.end()){
                if((*it) == val) return;
                else if((*it)<val){
                    ++it;
                    if(*it > val){ neighbors.insert(it, val); return; }
                    --it;
                }
                ++it;
            }
            neighbors.emplace_back(val);
            return;
        }

        size_t size() const { return neighbors.size(); }
    };
    
    template<typename name>
    class Graph
    {
    private:
        int index = 0;
        std::vector<HeadNode*> data;
        std::vector<name> names;
        boost::unordered_map<name, int> NameMap;
    public:
        Graph(){}
        ~Graph(){
            for(size_t i=0;i<data.size();i++){
                delete data[i];
                data[i] = nullptr;
            }
        }

        int newNode(const name& n){ 
            if(NameMap.find(n)!=NameMap.end()){ return NameMap[n]; } 
            else{
                int tmp = index;
                ++index;
                HeadNode* t = new HeadNode(tmp);
                NameMap.insert(std::pair<name, int>(n, tmp));
                data.emplace_back(t);
                names.emplace_back(n);
                return tmp;
            }
        }

        size_t size() const { return data.size(); }
        size_t size(int v) const { return data[v]->size(); }
        size_t size(const name& n) {
            if(NameMap.find(n)!=NameMap.end()){
                int tmp = NameMap[n];
                return data[tmp]->size(); 
            }
            else return 0; 
        }

        void addEdge(const name& n1, const name& n2){
            int node1 = -1, node2 = -1;
            if(NameMap.find(n1)!=NameMap.end()) node1 = NameMap[n1];
            else node1 = newNode(n1);
            if(NameMap.find(n2)!=NameMap.end()) node2 = NameMap[n2];
            else node2 = newNode(n2);

            data[node1]->addNeighbor(node2);
        }

        void print(){
            for(size_t i=0;i<data.size();i++){
                std::cout<<names[i]<<": ";
                std::list<int>::iterator it = data[i]->neighbors.begin();
                while(it!=data[i]->neighbors.end()){
                    std::cout<<*it<<" ";
                    ++it;
                }
                std::cout<<std::endl;
            }
        }

    };

} // namespace ismt




#endif