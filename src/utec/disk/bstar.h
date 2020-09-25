#pragma once

#include "pagemanager.h"
#include <memory>
#include <stack>

namespace utec {

    namespace disk {

        template <class T, int BSTAR_ORDER>
        class bstar;

        template <class T, int BSTAR_ORDER>
        class Node;

        template <class T, int BSTAR_ORDER = 3>
        class bstariterator {
        private:
            template <int SIZE = BSTAR_ORDER>
            using Node = utec::disk::Node<T, SIZE>;

            enum blocksize {
                F_BLOCK = (2*BSTAR_ORDER-2)/3,
            };
          
            std::shared_ptr<pagemanager> pm;

            std::stack<std::pair<long,int>> q;

            int index;
            long node_id;
        public:
            bstariterator(std::shared_ptr<pagemanager> &pm) : 
                pm(pm), node_id(-1), index(0){}

            bstariterator(std::shared_ptr<pagemanager> &pm, long node_id) : 
                pm(pm), node_id(node_id), index(0) {
                
                Node<2*F_BLOCK> n = read_root();
                if(n.children[0]){
                    this->q.push({n.page_id,0});
                    Node<> nn = read_node(n.children[0]);
                    while(nn.children[0]){
                        this->q.push({nn.page_id,0});
                        nn = read_node(nn.children[0]);
                    }
                    this->node_id = nn.page_id;
                } else {
                    this->node_id = n.page_id;
                }
                this->index = 0;
            
            }
         
            bstariterator(std::shared_ptr<pagemanager> &pm, const bstariterator& other): 
                pm(pm), node_id(other.node_id), index(other.index), q(other.q) {}

            void find(const T &key) {
                Node<2*F_BLOCK> n = read_root();
                int lb = 0;
                while (lb < n.count && n.keys[lb] < key) {
                    lb++;
                }

                if(n.keys[lb] == key){
                    node_id = n.page_id;
                    index = lb;
                    return;
                }

                if(!n.children[lb]){
                    node_id = -1;
                    index = 0;
                    return;
                }

                if(lb < n.count) q.push({n.page_id, lb});

                Node<> nn = read_node(n.children[lb]);
                lb = 0;
                while (lb < nn.count && nn.keys[lb] < key) {
                    lb++;
                }

                if(nn.keys[lb] == key){
                    node_id = nn.page_id;
                    index = lb;
                    return;
                }

                while(nn.children[lb]){
                    if(lb < nn.count) q.push({nn.page_id, lb});
                    nn = read_node(nn.children[lb]);
                    lb = 0; 
                    while (lb < nn.count && nn.keys[lb] < key) {
                        lb++;
                    }
                    if(nn.keys[lb] == key) break;
                }

                if(nn.keys[lb] != key){
                    node_id = -1;
                    index = 0;
                    q.empty();
                } else {
                    node_id = nn.page_id;
                    index = lb;
                }
            }

            Node<> read_node(long page_id) {
                Node<> n{-1};
                pm->recover(page_id, n);
                return n;
            }

            Node<2*F_BLOCK> read_root() {
                Node<2*F_BLOCK> n{-1};
                pm->recover(1, n);
                return n;
            }
          
            bstariterator& operator=(bstariterator other) { 
                this->node_id = other.node_id;
                this->index = other.index;
                return *this;
            }

            bstariterator& operator++() {
                Node<> n;
                Node<2*F_BLOCK> r;
                if(this->node_id == 1){
                    r = read_root();
                } else {
                    n = read_node(this->node_id);
                }
                if ((this->node_id != 1 && n.children[index+1])
                    || (this->node_id == 1 && r.children[index+1])) {

                    if(this->node_id != 1 && index+1 < n.count
                        || this->node_id == 1 && index+1 < r.count){
                        q.push({node_id,index+1});
                    }

                    long id;
                    Node<> nn;
                    if(this->node_id != 1)
                        nn = read_node(n.children[++index]);
                    else 
                        nn = read_node(r.children[++index]);
                    while(nn.children[0]){
                        q.push({nn.page_id,0});
                        id = nn.children[0];
                        nn = read_node(id);
                    }

                    this->node_id = nn.page_id;
                    this->index = 0;
                } else if ((this->node_id != 1 && !(index < n.count-1))
                    || (this->node_id == 1 && !(index < r.count-1))) {
                    if(q.empty()){
                        node_id = -1;
                        index = 0;
                        return *this;
                    }
                    node_id = q.top().first;
                    index = q.top().second;
                    q.pop();
                } else {
                    this->index++;
                }

                return *this;
            }

            bstariterator& operator++(int) { 
                bstariterator it(pm, *this);
                ++(*this);
                return it; 
            }

            bool operator==(const bstariterator& other) { 
                return (this->node_id == other.node_id) && (this->index == other.index); 
            }

            bool operator!=(const bstariterator& other) {
                return !((*this) == other);
            } 

            T operator*() { 
                if(this->node_id == 1){
                    Node<2*F_BLOCK> n = read_root();
                    return n.keys[index]; 
                } else {
                    Node<> n = read_node(this->node_id);
                    return n.keys[index]; 
                }
            }

            long get_page_id() {
                if(this->node_id == 1){
                    Node<2*F_BLOCK> n = read_root();
                    return n.children[index]; 
                } else {
                    Node<> n = read_node(this->node_id);
                    return n.children[index]; 
                }
            }
        };


        template <class T, int BSTAR_ORDER = 3>
        class Node {
        public:
            long page_id = -1;
            long count = 0;

            T keys[BSTAR_ORDER + 1];
            long children[BSTAR_ORDER + 2];

            Node() {}

            Node(long page_id) : page_id{page_id} {
                count = 0;
                for (int i = 0; i < BSTAR_ORDER + 2; i++) {
                    children[i] = 0;
                }
            }

            void insert_in_node(int pos, const T &value) {
                int j = count;
                while (j > pos) {
                    keys[j] = keys[j - 1];
                    children[j + 1] = children[j];
                    j--;
                }
                keys[j] = value;

                children[j + 1] = children[j];

                count++;
            }
        };  


        template <class T, int BSTAR_ORDER = 3>
        class bstar {
        public:
            template <int SIZE = BSTAR_ORDER>
            using Node = utec::disk::Node<T, SIZE>;

            typedef bstariterator<T, BSTAR_ORDER> iterator;

            enum state {
                BT_OVERFLOW,
                BT_UNDERFLOW,
                NORMAL,
            };

            enum blocksize {
                F_BLOCK = (2*BSTAR_ORDER-2)/3,
                S_BLOCK = (2*BSTAR_ORDER-1)/3,
                T_BLOCK = (2*BSTAR_ORDER)/3,
                H_BLOCK = (4*BSTAR_ORDER)/3,
            };

            struct Metadata {
                long root_id{1};
                long count{0};
            } header;

        private:
            std::shared_ptr<pagemanager> pm;

            Node<> new_node() {
                header.count++;
                Node<> ret{header.count+1};
                pm->save(0, header);
                return ret;
            }

            Node<> read_node(long page_id) {
                Node<> n{-1};
                pm->recover(page_id, n);
                return n;
            }

            Node<2*F_BLOCK> read_root() {
                Node<2*F_BLOCK> n{-1};
                pm->recover(1, n);
                return n;
            }

            template <int SIZE>
            bool write_node(long page_id, Node<SIZE> n) { 
                pm->save(page_id, n);
            }

            template <int SIZE>
            void rotateLeft(Node<SIZE> &node, Node<> &n1, Node<> &n2, int pos){
                n1.insert_in_node(n1.count, node.keys[pos]);
                n1.children[n1.count] = n2.children[0];
                node.keys[pos] = n2.keys[0];
                n2.count--;

                int i;
                for (i=0; i < n2.count; i++) {
                    n2.keys[i] = n2.keys[i + 1];
                    n2.children[i] = n2.children[i + 1];
                }
                n2.children[i] = n2.children[i + 1];

                write_node(node.page_id, node);
                write_node(n1.page_id, n1);
                write_node(n2.page_id, n2);
            }

            template <int SIZE>
            void rotateRight(Node<SIZE> &node, Node<> &n1, Node<> &n2, int pos){
                n1.insert_in_node(0, node.keys[pos]);
                n1.children[0] = n2.children[n2.count--];
                node.keys[pos] = n2.keys[n2.count];

                write_node(node.page_id, node);
                write_node(n1.page_id, n1);
                write_node(n2.page_id, n2);
            }

            template <int SIZE>
            void split(Node<SIZE> &node, int idx){
                int fidx, sidx;
                if(idx < node.count){
                    fidx = idx;
                    sidx = idx+1;
                } else {
                    fidx = idx-1;
                    sidx = idx;
                }
                Node<> fnode = read_node(node.children[fidx]);
                Node<> snode = read_node(node.children[sidx]);
                Node<> tnode = new_node();

                int i, s=snode.count-T_BLOCK;
                for (i = 0; i < T_BLOCK; i++) {
                    tnode.keys[i] = snode.keys[s+i];
                    tnode.children[i] = snode.children[s+i];
                }
                tnode.children[i] = snode.children[s+i];
                snode.count -= T_BLOCK; tnode.count += T_BLOCK;

                snode.insert_in_node(0, node.keys[fidx]);
                snode.children[0] = fnode.children[fnode.count];

                node.insert_in_node(sidx, snode.keys[--snode.count]);
                node.children[sidx] = node.children[sidx+1];
                node.children[sidx+1] = tnode.page_id;

                while (fnode.count > F_BLOCK+1) {
                    snode.insert_in_node(0, fnode.keys[--fnode.count]);
                    snode.children[0] = fnode.children[fnode.count];
                }

                node.keys[fidx] = fnode.keys[--fnode.count];

                write_node(node.page_id, node);
                write_node(fnode.page_id, fnode);
                write_node(snode.page_id, snode);
                write_node(tnode.page_id, tnode);
            }

            void split_root(Node<2*F_BLOCK> &root){
                Node<> left = new_node();
                Node<> right = new_node();
                T middle = root.keys[F_BLOCK];
                int i;

                for (i = 0; i < F_BLOCK; i++) {
                    left.keys[i] = root.keys[i];
                    left.children[i] = root.children[i];
                }
                left.children[i] = root.children[i];
                left.count = F_BLOCK; root.count -= F_BLOCK;

                for (i = 0; i < F_BLOCK; i++) {
                    right.keys[i] = root.keys[F_BLOCK+i+1];
                    right.children[i] = root.children[F_BLOCK+i+1];
                }
                right.children[i] = root.children[F_BLOCK+i+1];
                right.count = F_BLOCK; root.count -= F_BLOCK;

                root.keys[0] = middle;
                root.children[0] = left.page_id;
                root.children[1] = right.page_id;

                write_node(left.page_id, left);
                write_node(right.page_id, right);
            }

            template <int SIZE>
            int insert(T data, Node<SIZE> &node){
                int i;
                for(i=0; i<node.count; ++i)
                    if(data<=node.keys[i]) break;
                if(node.children[i]){
                    Node<> prev = read_node(node.children[i-1]);
                    Node<> temp = read_node(node.children[i]);
                    Node<> next = read_node(node.children[i+1]);
                    int status = insert(data,temp);
                    if(status == BT_OVERFLOW){
                        if(i<node.count && next.count < BSTAR_ORDER-1){
                            rotateRight(node,next,temp,i);
                            return NORMAL;
                        }
                        if(i && prev.count < BSTAR_ORDER-1){
                            rotateLeft(node,prev,temp,i-1);
                            return NORMAL;
                        }
                        split(node,i);
                    }
                } else {
                    node.insert_in_node(i, data);
                    write_node(node.page_id, node);
                }
                if(node.count==BSTAR_ORDER) {
                    return BT_OVERFLOW;
                }
                return NORMAL;
            }

            template <int SIZE>
            void merge(Node<SIZE> &node, Node<> &n1, Node<> &n2, Node<> &n3, int pos){

                T keys[2*(BSTAR_ORDER+1)];
                long children[2*(BSTAR_ORDER+2)];

                // JOIN TO SPLIT

                int i, j;
                for(i=0; i<n1.count; i++){
                    keys[i] = n1.keys[i];
                    children[i] = n1.children[i];
                }
                children[i] = n1.children[i];

                keys[i] = node.keys[pos];
                i++;

                for(j=0; j<n2.count; i++, j++){
                    keys[i] = n2.keys[j];
                    children[i] = n2.children[j];
                }
                children[i] = n2.children[j];

                keys[i] = node.keys[pos+1];
                i++;

                for(j=0; j<n3.count; i++,j++){
                    keys[i] = n3.keys[j];
                    children[i] = n3.children[j];
                }
                children[i] = n3.children[j];

                // SPLIT

                int size = i;

                for(i=0; i<BSTAR_ORDER-1; i++){
                    n1.keys[i] = keys[i];
                    n1.children[i] = children[i];
                }
                n1.children[i] = children[i];
                n1.count = BSTAR_ORDER-1;

                node.keys[pos] = keys[i];
                i++;

                for(j=0; i<size; i++, j++){
                    n2.keys[j] = keys[i];
                    n2.children[j] = children[i];
                }
                n2.children[j] = children[i];
                n2.count = size - BSTAR_ORDER;

                // ERASE 3RD NODE

                for(i=pos+2; i<node.count; i++){
                    node.keys[i-1] = node.keys[i];
                    node.children[i] = node.children[i+1];
                }
                node.count--;
            }

            template <int SIZE>
            void mergeRoot(Node<SIZE> &node){
                Node<> n1 = read_node(node.children[0]); 
                Node<> n2 = read_node(node.children[1]);

                node.keys[n1.count] = node.keys[0];

                int i, j;
                for(i=0; i<n1.count; i++){
                    node.keys[i] = n1.keys[i];
                    node.children[i] = n1.children[i];
                }
                node.children[i] = n1.children[i];
                node.count += n1.count;
                i++;

                for(j=0; j<n2.count; i++){
                    node.keys[i] = n2.keys[j];
                    node.children[i] = n2.children[j++];
                }
                node.children[i] = n2.children[j];
                node.count += n2.count;
            }


            template <int SIZE>
            bool remove(T data, T* &temp, Node<SIZE> &node){
                int i;
                
                for(i=0; i<node.count; ++i){
                    if(data<=node.keys[i]) break;
                }

                if(!node.children[i]){
                    if(data != node.keys[i] && !temp) 
                        return false;
                    if(i==node.count) --i;
                    if(temp && *temp != node.keys[i]) {
                        swap(*temp,node.keys[i]);
                    }
                    for(int idx=i; idx<node.count; idx++){
                        node.keys[idx] = node.keys[idx+1];
                    }
                    node.count--;
                    write_node(node.page_id, node);
                    return true;
                }

                if(i<node.count && data == node.keys[i]) temp=&node.keys[i];
                Node<> n = read_node(node.children[i]);
                if(!remove(data,temp,n)) return false;
                
                write_node(n.page_id, n);
                write_node(node.page_id, node);
                
                //NEW MODIFICATIONS

                auto size = n.count;

                if(size < F_BLOCK){

                    if(i==0) {
                        Node<> next, next2;
                        next = read_node(node.children[i+1]);    
                        if(node.count > 1) next2 = read_node(node.children[i+2]);

                        auto size_r = next.count;

                        if(size_r > F_BLOCK){

                            rotateLeft(node,n,next,i);

                            write_node(node.page_id, node);
                            write_node(n.page_id, n);
                            write_node(next.page_id, next);
                        
                        } else if(node.count > 1 && next2.count > F_BLOCK){

                            rotateLeft(node,next,next2,i+1);
                            size = n.count;
                            rotateLeft(node,n,next,i);

                            write_node(node.page_id, node);
                            write_node(n.page_id, n);
                            write_node(next.page_id, next);
                            write_node(next2.page_id, next2);

                        } else {

                            if(node.page_id == 1 && node.count == 1) {
                                mergeRoot(node);
                            } else {
                                merge(node, n, next, next2, i);
                                write_node(n.page_id, n);
                                write_node(next.page_id, next);
                                write_node(next2.page_id, next2);
                            }
                            write_node(node.page_id, node);

                        }

                    } else if(i==node.count){
                        Node<> prev, prev2;
                        prev = read_node(node.children[i-1]);    
                        if(node.count > 1) prev2 = read_node(node.children[i-2]);

                        auto size_l = prev.count;

                        if(size_l > F_BLOCK){

                            rotateRight(node,n,prev,i-1);

                            write_node(node.page_id, node);
                            write_node(n.page_id, n);
                            write_node(prev.page_id, prev);
                        
                        } else if(node.count > 1 && prev2.count > F_BLOCK){
                            
                            auto size_l2 = prev2.count;

                            rotateRight(node,prev,prev2,i-2);
                            size = n.count;
                            rotateRight(node,n,prev,i-1);

                            write_node(node.page_id, node);
                            write_node(n.page_id, n);
                            write_node(prev.page_id, prev);
                            write_node(prev2.page_id, prev2);

                        } else {

                            if(node.page_id == 1 && node.count == 1) {
                                mergeRoot(node);
                            } else {
                                merge(node, prev2, prev, n, i-2);
                                write_node(n.page_id, n);
                                write_node(prev.page_id, prev);
                                write_node(prev2.page_id, prev2);
                            }
                            write_node(node.page_id, node);

                        }

                    } else {

                        Node<> prev, next;
                        prev = read_node(node.children[i-1]);    
                        next = read_node(node.children[i+1]);

                        auto size_l = prev.count;
                        auto size_r = next.count;

                        if(size_l > F_BLOCK){

                            rotateRight(node,n,prev,i-1);

                            write_node(node.page_id, node);
                            write_node(n.page_id, n);
                            write_node(prev.page_id, prev);
                        
                        } else if(size_r > F_BLOCK){

                            rotateLeft(node,n,next,i);

                            write_node(node.page_id, node);
                            write_node(n.page_id, n);
                            write_node(next.page_id, next);

                        } else {

                            if(node.page_id == 1 && node.count == 1) {
                                mergeRoot(node);
                            } else {
                                merge(node, prev, n, next, i-1);
                                write_node(n.page_id, n);
                                write_node(next.page_id, next);
                                write_node(prev.page_id, prev);
                            }
                            write_node(node.page_id, node);
                        }
                    }
                }
                return true;
            }



            template <int SIZE>
            void print_tree(Node<SIZE> &ptr, int level) {
                int i;
                for (i = ptr.count - 1; i >= 0; i--) {
                    if (ptr.children[i + 1]) {
                        Node<> child = read_node(ptr.children[i + 1]);
                        print_tree(child, level + 1);
                    }
                    for (int k = 0; k < level; k++) {
                        std::cout << "    ";
                    }
                    std::cout << ptr.keys[i] << "\n";
                }
                if (ptr.children[i + 1]) {
                    Node<> child = read_node(ptr.children[i + 1]);
                    print_tree(child, level + 1);
                }
            }

            template <int SIZE>
            void print(Node<SIZE> &ptr, int level, std::ostream& out) {
                int i;
                for (i = 0; i < ptr.count; i++) {
                    if (ptr.children[i]) {
                        Node<> child = read_node(ptr.children[i]);
                        print(child, level + 1, out);
                    }
                    out << ptr.keys[i];
                }
                if (ptr.children[i]) {
                    Node<> child = read_node(ptr.children[i]);
                    print(child, level + 1, out);
                }
            }

        public:
            bstar(std::shared_ptr<pagemanager> pm) : pm{pm} {
                if (pm->is_empty()) {
                    Node<2*F_BLOCK> root{header.root_id};
                    pm->save(root.page_id, root);

                    header.count++;

                    pm->save(0, header);
                } else {
                    pm->recover(0, header);
                }
            }

            void insert(T k) {
                Node<2*F_BLOCK> root = read_root();
                insert(k,root);
                if(root.count > F_BLOCK*2){
                    split_root(root);
                }
                write_node(root.page_id, root);
            }

            bool remove(T k) {
                T *temp=0;
                Node<2*F_BLOCK> root = read_root();
                return remove(k,temp,root);
            }

            iterator find(const T &key) {
                iterator it(this->pm);
                it.find(key);
                return it;
            }

            iterator begin() {
                iterator it(this->pm, header.root_id);
                return it;
            }

            iterator end() {
                iterator it(this->pm);
                return it;
            }

            void print_tree() {
                Node<2*F_BLOCK> root = read_root();
                print_tree(root, 0);
                std::cout << "________________________\n";
            }
            
            void print(std::ostream& out) {
                Node<2*F_BLOCK> root = read_root();
                print(root, 0, out);
            }

        };

    } // namespace disk

} // namespace utec
