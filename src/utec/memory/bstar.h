#pragma once

#include <iostream>
#include <vector>
#include <algorithm>

using namespace std;

namespace utec {

    namespace memory {

        template <class T, int BTREE_ORDER = 3>
        class bstar {
        private:
            enum state {
                BT_OVERFLOW,
                BT_UNDERFLOW,
                NORMAL,
            };

            enum blocksize {
                F_BLOCK = (2*BTREE_ORDER-2)/3,
                S_BLOCK = (2*BTREE_ORDER-1)/3,
                T_BLOCK = (2*BTREE_ORDER)/3,
            };

            struct Node {
                vector<T> keys;
                vector<Node*> children;
                bool isLeaf;

                Node(bool isLeaf): isLeaf(isLeaf){}
            };

            Node* root;

            bool find(T data, Node* &node, int &i){
                while(node){
                    for(i=0; i<node->keys.size(); ++i) {
                        if(data==node->keys[i]) return true;
                        else if(data<node->keys[i]) break;
                    }
                    if(!node->isLeaf) node = node->children[i];
                    else node = 0;
                }
                return false;
            }

            void insKeys(Node* &node1, Node* &node2, int pos){
                node1->keys.insert(node1->keys.begin(),node2->keys.begin()+pos+1,node2->keys.end());
                node2->keys.erase(node2->keys.begin()+pos,node2->keys.end());
            }

            void inschildren(Node* &node1, Node* &node2, int pos){
                node1->children.insert(node1->children.begin(),node2->children.begin()+pos+1,node2->children.end());
                node2->children.erase(node2->children.begin()+pos+1,node2->children.end());
            }

            void rotate(Node* &node, Node* n1, Node* n2, int n3, int pos1, int pos2, int pos3, int pos4){
                n1->keys.insert(n1->keys.begin()+pos2,node->keys[n3]);
                node->keys[n3] = n2->keys[pos1];
                n2->keys.erase(n2->keys.begin()+pos1);
                if(!n1->isLeaf) {
                    n1->children.insert(n1->children.begin()+pos2+pos3,n2->children[pos1+pos4]);
                    n2->children.erase(n2->children.begin()+pos1+pos4);
                }
            }

            void merge(Node* node, Node* n1, Node* n2, Node* n3, int pos){

                vector<T> keys;
                vector<Node*> children;

                // JOIN TO SPLIT

                keys.insert(keys.end(), n1->keys.begin(), n1->keys.end());
                children.insert(children.end(), n1->children.begin(), n1->children.end());

                keys.insert(keys.end(), node->keys[pos]);

                keys.insert(keys.end(), n2->keys.begin(), n2->keys.end());
                children.insert(children.end(), n2->children.begin(), n2->children.end());

                keys.insert(keys.end(), node->keys[pos+1]);

                keys.insert(keys.end(), n3->keys.begin(), n3->keys.end());
                children.insert(children.end(), n3->children.begin(), n3->children.end());

                // EMPTY VECTORS KEYS AND CHILDREN

                n1->keys.clear(); n1->children.clear();
                n2->keys.clear(); n2->children.clear();

                // SPLIT

                n1->keys.insert(n1->keys.end(), keys.begin(), keys.begin() + BTREE_ORDER - 1);
                if(!children.empty())
                    n1->children.insert(n1->children.end(), children.begin(), children.begin() + BTREE_ORDER);

                node->keys[pos] = keys[BTREE_ORDER - 1];

                n2->keys.insert(n2->keys.end(), keys.begin() + BTREE_ORDER, keys.end());
                if(!children.empty())
                    n2->children.insert(n2->children.end(), children.begin() + BTREE_ORDER, children.end());

                // ERASE 3RD NODE

                delete node->children[pos + 2]; 
                node->children.erase(node->children.begin() + pos + 2);
                node->keys.erase(node->keys.begin() + pos + 1);

            }

            void mergeRoot(Node* node, Node* n1, Node* n2){

                n1->keys.insert(n1->keys.end(), node->keys[0]);
                n1->keys.insert(n1->keys.end(), n2->keys.begin(), n2->keys.end());
                n1->children.insert(n1->children.end(), n2->children.begin(), n2->children.end());

                delete node->children[1];
                delete root;

                this->root = n1;

            }

            void split(Node* node, int idx){
                int fidx, sidx;
                Node *fnode, *snode, *tnode;
                if(idx < node->children.size()-1){
                    fidx = idx;
                    sidx = idx+1;
                } else {
                    fidx = idx-1;
                    sidx = idx;
                }
                fnode = node->children[fidx];
                snode = node->children[sidx];
                tnode = new Node(snode->isLeaf);

                node->children.insert(node->children.begin()+sidx+1,tnode);

                snode->keys.insert(snode->keys.begin(),node->keys[fidx]);
                node->keys[fidx] = *(fnode->keys.begin()+F_BLOCK);

                insKeys(snode,fnode,F_BLOCK);
                if(!snode->isLeaf) {
                    inschildren(snode,fnode,F_BLOCK);
                }

                node->keys.insert(node->keys.begin()+sidx,*(snode->keys.begin()+S_BLOCK));

                insKeys(tnode,snode,S_BLOCK);
                if(!tnode->isLeaf) {
                    inschildren(tnode,snode,S_BLOCK);
                }
            }

            int insert(T &data, Node* &node){
                int i;
                for(i=0; i<node->keys.size(); ++i)
                    if(data<=node->keys[i]) break;
                if(!node->isLeaf){
                    auto temp = node->children[i];
                    int status = insert(data,temp);
                    if(status == BT_OVERFLOW){
                        if(i<node->children.size()-1 && node->children[i+1]->keys.size() < BTREE_ORDER-1){
                            rotate(node,node->children[i+1],node->children[i],i,
                                   node->children[i]->keys.size()-1,0,0,1);
                            return NORMAL;
                        }
                        if(i && node->children[i-1]->keys.size() < BTREE_ORDER-1){
                            rotate(node,node->children[i-1],node->children[i],i-1,0,
                                   node->children[i-1]->keys.size(),1,0);
                            return NORMAL;
                        }
                        split(node,i);
                    }
                } else node->keys.insert(node->keys.begin()+i,data);
                if(node->keys.size()==BTREE_ORDER){
                    return BT_OVERFLOW;
                }
                return NORMAL;
            }

            bool remove(T data, T* &temp, Node* &node){
                int i;
                for(i=0; i<node->keys.size(); ++i){
                    if(data<=node->keys[i]) break;
                }
                if(node->isLeaf){
                    if(data != node->keys[i] && !temp) return false;
                    if(i==node->keys.size()) --i;
                    if(temp && *temp != node->keys[i]) swap(*temp,node->keys[i]);
                    node->keys.erase(node->keys.begin()+i);
                    return true;
                }
                if(i<node->keys.size() && data == node->keys[i]) temp=&node->keys[i];
                if(!remove(data,temp,node->children[i])) return false;
                auto size=node->children[i]->keys.size();
                
                //NEW MODIFICATIONS

                if(size < S_BLOCK){

                    if(i==0) {

                        auto size_r=node->children[i+1]->keys.size();

                        if(size_r > F_BLOCK){

                            rotate(node,node->children[i],node->children[i+1],i,0,size,1,0);
                        
                        } else if(node->children.size() > 2 && node->children[i+2]->keys.size() > F_BLOCK){

                            rotate(node,node->children[i+1],node->children[i+2],i+1,0,size_r,1,0);
                            size=node->children[i]->keys.size();
                            rotate(node,node->children[i],node->children[i+1],i,0,size,1,0);

                        } else {

                            if(node == root && node->keys.size() == 1) {
                                mergeRoot(node, node->children[0], node->children[1]);
                            } else {
                                merge(node, node->children[i], node->children[i+1], node->children[i+2], i);
                            }

                        }

                    } else if(i+1==node->children.size()){

                        auto size_l=node->children[i-1]->keys.size();

                        if(size_l > F_BLOCK){
                            auto size_l=node->children[i-1]->keys.size();

                            rotate(node,node->children[i],node->children[i-1],i-1,size_l-1,0,0,1);
                        
                        } else if(node->children.size() > 2 && node->children[i-2]->keys.size() > F_BLOCK){
                            auto size_l2=node->children[i-2]->keys.size();

                            rotate(node,node->children[i-1],node->children[i-2],i-2,size_l2-1,0,0,1);
                            size_l=node->children[i-1]->keys.size();
                            rotate(node,node->children[i],node->children[i-1],i-1,size_l-1,0,0,1);

                        } else {

                            if(node == root && node->keys.size() == 1) {
                                mergeRoot(node, node->children[0], node->children[1]);
                            } else {
                                merge(node, node->children[i-2], node->children[i-1], node->children[i], i-2);
                            }

                        }

                    } else {

                        auto size_l=node->children[i-1]->keys.size();

                        if(node->children[i-1]->keys.size() > F_BLOCK){

                            rotate(node,node->children[i],node->children[i-1],i-1,size_l-1,0,0,1);
                        
                        } else if(node->children[i+1]->keys.size() > F_BLOCK){

                            rotate(node,node->children[i],node->children[i+1],i,0,size,1,0);

                        } else {

                            if(node == root && node->keys.size() == 1) {
                                mergeRoot(node, node->children[0], node->children[1]);
                            } else {
                                merge(node, node->children[i-1], node->children[i], node->children[i+1], i-1);
                            }

                        }

                    }
                }
                return true;
            }

            void traverseInOrder(Node* node) {
                int i;
                for(i=0; i<node->keys.size(); ++i){
                    if(!node->isLeaf) traverseInOrder(node->children[i]);
                    cout << node->keys[i] << ' ';
                }
                if(!node->isLeaf) traverseInOrder(node->children[i]);
            }

            void deleteAll(Node* node){
                int i;
                for(i=0; i<node->keys.size(); ++i){
                    if(!node->isLeaf) deleteAll(node->children[i]);
                }
                if(!node->isLeaf) deleteAll(node->children[i]);
                delete node;
            }

            void print_tree(Node *ptr, int level) {
                int i;
                for (i = ptr->keys.size() - 1; i >= 0; i--) {
                    if(!ptr->children.empty())
                        print_tree(ptr->children[i + 1], level + 1);

                    for (int k = 0; k < level; k++) {
                        std::cout << "    ";
                    }
                    std::cout << ptr->keys[i] << "\n";
                }
                if(!ptr->children.empty())
                    print_tree(ptr->children[i + 1], level + 1);
            }

        public:
            bstar() : root(nullptr) {
                root = new Node(BTREE_ORDER);
            }

            bool search(T k) {
                auto temp = root; int i;
                return find(k,temp,i);
            }

            void insert(T k) {
                auto temp = root;
                insert(k,temp);
                if(root->keys.size() > F_BLOCK*2){
                    Node* newRoot = new Node(false);
                    Node* newNode = new Node(root->isLeaf);

                    newRoot->keys.push_back(temp->keys[F_BLOCK]);
                    newRoot->children.push_back(root);
                    newRoot->children.push_back(newNode);

                    insKeys(newNode,root,F_BLOCK);
                    if(!newNode->isLeaf) {
                        inschildren(newNode,root,F_BLOCK);
                    }

                    root = newRoot;
                }
            }

            bool remove(T k) {
                T *temp=0;
                Node *node=root;
                return remove(k,temp,node);
            }

            void print() {
                traverseInOrder(root);
                cout << endl;
            }

            void print_tree() {
                print_tree(root, 0);
                std::cout << "________________________\n";
            }

            ~bstar(){
                deleteAll(root);
            }
        };
    }
}