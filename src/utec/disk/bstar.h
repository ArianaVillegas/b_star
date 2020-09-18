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
            friend class bstar<T, BSTAR_ORDER>;

            typedef utec::disk::Node<T, BSTAR_ORDER> Node;
          
            std::shared_ptr<pagemanager> pm;

            std::stack<std::pair<long,int>> q;

            int index;
            long node_id;
        public:
            bstariterator(std::shared_ptr<pagemanager> &pm, long node_id) : 
                pm(pm), node_id(node_id), index(0) {
                    if(node_id < 1) return;
                    auto n = read_node(this->node_id);
                    q.push({n.page_id,0});
                    while(n.children[0]){
                        n = read_node(n.children[0]);
                        q.push({n.page_id,0});
                    }
                    q.pop();
                    this->index = 0;
                    this->node_id = n.page_id;
                }

            bstariterator(std::shared_ptr<pagemanager> &pm, long node_id, const T &key) : 
                pm(pm), node_id(node_id) {
                    auto n = read_node(this->node_id);
                    int lb = 0;
                    while (lb < n.count && n.keys[lb] < key) {
                        lb++;
                    }
                    q.push({n.page_id, lb});
                    if(n.keys[lb] != key){
                        while(n.children[lb]){
                            n = read_node(n.children[lb]);
                            lb = 0; 
                            while (lb < n.count && n.keys[lb] < key) {
                                lb++;
                            }
                            q.push({n.page_id, lb});
                            if(n.keys[lb] == key) break;
                        }
                    }
                    q.pop();
                    this->index = lb;
                    this->node_id = n.page_id;
                }
         
            bstariterator(std::shared_ptr<pagemanager> &pm, const bstariterator& other): 
                pm(pm), node_id(other.node_id), index(other.index), q(other.q) {}

            Node read_node(long page_id) {
                Node n{-1};
                pm->recover(page_id, n);
                return n;
            }
          
            bstariterator& operator=(bstariterator other) { 
                this->node_id = other.node_id;
                this->index = other.index;
                return *this;
            }

            bstariterator& operator++() {
                Node n = read_node(this->node_id);

                if (n.children[index+1]) {
                    long id;
                    n = read_node(n.children[++index]);
                    while(n.children[0]){
                        q.push({n.page_id,0});
                        id = n.children[0];
                        n = read_node(id);
                    }

                    this->node_id = n.page_id;
                    this->index = 0;
                } else if (!(index < n.count-1)) {
                    if(q.empty()){
                        node_id = -1;
                        index = 0;
                        return *this;
                    }
                    node_id = q.top().first;
                    index = q.top().second;
                    n = read_node(this->node_id);
                    q.pop();
                    if(index+1 < n.count)
                        q.push({node_id,index+1});
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
                Node n = read_node(this->node_id);
                return n.keys[index]; 
            }

            long get_page_id() {
                Node n = read_node(this->node_id);
                return n.pages[index];
            }
        };


        template <class T, int BSTAR_ORDER = 3>
        class Node {
        public:
            long page_id = -1;
            long count = 0;

            T keys[2*(2*BSTAR_ORDER-2)/3 + 1];
            long children[2*(2*BSTAR_ORDER-2)/3 + 2];

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
            typedef utec::disk::Node<T, BSTAR_ORDER> Node;
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

            Node new_node() {
                header.count++;
                Node ret{header.count+2};
                pm->save(0, header);
                return ret;
            }

            Node read_node(long page_id) {
                Node n{-1};
                pm->recover(page_id, n);
                return n;
            }

            bool write_node(long page_id, Node n) { pm->save(page_id, n);}

            void rotateLeft(Node &node, Node &n1, Node &n2, int pos){
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

            void rotateRight(Node &node, Node &n1, Node &n2, int pos){
                n1.insert_in_node(0, node.keys[pos]);
                n1.children[0] = n2.children[n2.count--];
                node.keys[pos] = n2.keys[n2.count];

                write_node(node.page_id, node);
                write_node(n1.page_id, n1);
                write_node(n2.page_id, n2);
            }

            void split(Node &node, int idx){
                int fidx, sidx;
                if(idx < node.count){
                    fidx = idx;
                    sidx = idx+1;
                } else {
                    fidx = idx-1;
                    sidx = idx;
                }
                Node fnode = read_node(node.children[fidx]);
                Node snode = read_node(node.children[sidx]);
                Node tnode = new_node();

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

            void split_root(Node &root){
                Node left = new_node();
                Node right = new_node();
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

            int insert(T data, Node &node){
                int i;
                for(i=0; i<node.count; ++i)
                    if(data<=node.keys[i]) break;
                if(node.children[i]){
                    Node prev = read_node(node.children[i-1]);
                    Node temp = read_node(node.children[i]);
                    Node next = read_node(node.children[i+1]);
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

            void print_tree(Node &ptr, int level) {
                int i;
                for (i = ptr.count - 1; i >= 0; i--) {
                    if (ptr.children[i + 1]) {
                        Node child = read_node(ptr.children[i + 1]);
                        print_tree(child, level + 1);
                    }
                    for (int k = 0; k < level; k++) {
                        std::cout << "    ";
                    }
                    std::cout << ptr.keys[i] << "\n";
                }
                if (ptr.children[i + 1]) {
                    Node child = read_node(ptr.children[i + 1]);
                    print_tree(child, level + 1);
                }
            }

            void print(Node &ptr, int level, std::ostream& out) {
                int i;
                for (i = 0; i < ptr.count; i++) {
                    if (ptr.children[i]) {
                        Node child = read_node(ptr.children[i]);
                        print(child, level + 1, out);
                    }
                    out << ptr.keys[i];
                }
                if (ptr.children[i]) {
                    Node child = read_node(ptr.children[i]);
                    print(child, level + 1, out);
                }
            }

        public:
            bstar(std::shared_ptr<pagemanager> pm) : pm{pm} {
                if (pm->is_empty()) {
                    Node root{header.root_id};
                    pm->save(root.page_id, root);

                    header.count++;

                    pm->save(0, header);
                } else {
                    pm->recover(0, header);
                }
            }

            void insert(T k) {
                Node root = read_node(header.root_id);
                insert(k,root);
                if(root.count > F_BLOCK*2){
                    split_root(root);
                }
                write_node(root.page_id, root);
            }

            iterator find(const T &key) {
                Node ptr = this->read_node(header.root_id);
                int pos = 0;

                while (ptr.children[0]) {
                    pos = 0;
                    while (pos < ptr.count && ptr.keys[pos] < key) {
                        pos++;
                    }
                    if(ptr.keys[pos] == key) break;
                    ptr = this->read_node(ptr.children[pos]);
                }

                if(ptr.keys[pos] != key){
                    pos = 0;
                    while (pos < ptr.count && ptr.keys[pos] < key) {
                        pos++;
                    }
                }

                if(pos < ptr.count && ptr.keys[pos] == key){
                    iterator it(this->pm, header.root_id, key);
                    return it;
                } else {
                    iterator it(this->pm, -1);
                    return it;
                }
            }

            iterator begin() {
                iterator it(this->pm, header.root_id);
                return it;
            }

            iterator end() {
                iterator it(this->pm, -1);
                return it;
            }

            void print_tree() {
                Node root = read_node(header.root_id);
                print_tree(root, 0);
                std::cout << "________________________\n";
            }
            
            void print(std::ostream& out) {
                Node root = read_node(header.root_id);
                print(root, 0, out);
            }

        };

    } // namespace disk

} // namespace utec
