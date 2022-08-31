/* ************************************************************************
> File Name:     skiplist.h
> Author:        程序员Carl
> 微信公众号:    代码随想录
> Created Time:  Sun Dec  2 19:04:26 2018
> Description:   
 ************************************************************************/

#include <iostream> 
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <mutex>
#include <fstream>

#define STORE_FILE "store/dumpFile"

std::mutex mtx;     // mutex for critical section
std::string delimiter = ":";


//链表中的节点类
template<typename K, typename V> 
class Node {

public:
    
    Node() {} 

    Node(K k, V v, int); 

    ~Node();

    K get_key() const;

    V get_value() const;

    void set_value(V);
    
  
    //forward-存储当前结点在i层的下一个结点。
    Node<K, V> **forward;
//这是一个数组，存储当前节点，在上面每一层的next节点。
//比如:key为1的节点，在level=1层的next节点为key为2的节点，所以key为1的节点的forward[1] = key=2的node指针

/*
level 4:                            2:B
level 3:                            2:B
level 2:                            2:B                   4:D                6:F
level 1:               1:A          2:B                   4:D        5:E     6:F
level 0:               1:A          2:B         3:C       4:D        5:E     6:F
*/

    int node_level;//所在高度

private:
    K key;
    V value;
};

//节点的有参构造函数，
template<typename K, typename V> 
Node<K, V>::Node(const K k, const V v, int level) {
    this->key = k;
    this->value = v;
    this->node_level = level; 

    //开辟forward数组空间，并使用memset初始化
    this->forward = new Node<K, V>*[level+1];
    memset(this->forward, 0, sizeof(Node<K, V>*)*(level+1));
};

template<typename K, typename V> 
Node<K, V>::~Node() {
    delete []forward;
};

template<typename K, typename V> 
K Node<K, V>::get_key() const {
    return key;
};

template<typename K, typename V> 
V Node<K, V>::get_value() const {
    return value;
};

template<typename K, typename V> 
void Node<K, V>::set_value(V value) {
    this->value=value;
};

// 跳表类
template <typename K, typename V> 
class SkipList {

public: 
    SkipList(int);
    ~SkipList();
    int get_random_level();
    Node<K, V>* create_node(K, V, int);
    int insert_element(K, V);
    void display_list();
    bool search_element(K);
    void delete_element(K);
    void dump_file();
    void load_file();
    int size();

private:
    void get_key_value_from_string(const std::string& str, std::string* key, std::string* value);
    bool is_valid_string(const std::string& str);

private:    
    // 该跳表最大层数
    int _max_level;

    // 该跳表当前层数
    int _skip_list9998_level;

    // 跳表头节点
    Node<K, V> *_header;

    // 文件操作
    std::ofstream _file_writer;
    std::ifstream _file_reader;

    // 该跳表当前的元素数
    int _element_count;
};

//创建一个新节点
template<typename K, typename V>
Node<K, V>* SkipList<K, V>::create_node(const K k, const V v, int level) {
    Node<K, V> *n = new Node<K, V>(k, v, level);
    return n;
}

// Insert given key and value in skip list 
//返回1代表元素存在
//返回0代表插入成功
/* 
                           +------------+
                           |  insert 50 |
                           +------------+
level 4     +-->1+                                                      100
                 |
                 |                      insert +----+
level 3         1+-------->10+---------------> | 50 |          70       100
                                               |    |
                                               |    |
level 2         1          10         30       | 50 |          70       100
                                               |    |
                                               |    |
level 1         1    4     10         30       | 50 |          70       100
                                               |    |
                                               |    |
level 0         1    4   9 10         30   40  | 50 |  60      70       100
                                               +----+

*/

//插入元素
template<typename K, typename V>
int SkipList<K, V>::insert_element(const K key, const V value) {
    
    mtx.lock();
    Node<K, V> *current = this->_header;//先拿到头节点
    //头节点是"立体的"，即，它是每一层的头节点。因为有forward[]来控制从哪层出发。
    //而且我们是通过forward将各个节点相关联的。
  

    //创建update数组并进行初始化
    // create update array and initialize it 
    // update is array which put node that the node->forward[i] should be operated later
    Node<K, V> *update[_max_level+1];
    memset(update, 0, sizeof(Node<K, V>*)*(_max_level+1));  

    // 从跳表左上角开始查找-_skip_list_level为当前所存在的最大的层数
    for(int i = _skip_list_level; i >= 0; i--) {//控制当前所在层，从最高层到第0层
        //如果当前节点的forward数组不为空，即在当前层，存在当前节点，并且，存在next节点。
        //此循环成立条件如下:
        //除了上面说的那个条件，同时还要求，当前节点的key小于我们要查找的key,小于则继续往后面走。
        while(current->forward[i] != NULL && current->forward[i]->get_key() < key) {//是不是继续往后面走
            //通过下面这行不断的在当前行遍历，直到，到当前行的最后一个，或是找到了第一个大于等于要插入节点的节点
            current = current->forward[i]; //forward存储该节点在当前层的下一个节点
        }
        //每一层记录一个节点，这个节点的作用是？
        //存储小于要插入节点的最后一个节点
        update[i] = current;
    }

    //最终应该是将所有的节点都插入到第一层的中，即，forward[0]那层
    //到达第0层，并且当前的forward指针指向第一个大于等于待插入节点的节点。
    //即,准备插入到update[i]后面
    current = current->forward[0];//这个current用来判断要插入的节点存key是否存在

    // 存在即修改
    // 如果当前节点的key值和待插入节点的key相等，则说明待插入节点值存在，修改该节点的值，这里并没有进行修改。
    if (current != NULL && current->get_key() == key) {
        std::cout << "key: " << key << ", exists" << std::endl;
        mtx.unlock();
        return 1;
    }

    //如果current节点为null，这就意味着要将该元素应该插入到最后
    //如果current的key值和待插入的key不等，代表我们应该在update[0]和current之间插入该节点。
    if (current == NULL || current->get_key() != key ) {
        
        // 为当前要插入的节点生成一个随机层数
        int random_level = get_random_level();
        
        //如果随机出来的层数大于当前链表达到的层数，注意:不是最大层，而是当前的最高层。
        //更新层数，准备插入新元素。
        if (random_level > _skip_list_level) {
            for (int i = _skip_list_level+1; i < random_level+1; i++) {
                update[i] = _header;
            }
            _skip_list_level = random_level;
        }

        // 创建节点
        Node<K, V>* inserted_node = create_node(key, value, random_level);
        
        // 插入节点
        for (int i = 0; i <= random_level; i++) {
            //在每一层，对应的位置，将新加入的节点放入，并且将前后相关联。
            inserted_node->forward[i] = update[i]->forward[i];
            update[i]->forward[i] = inserted_node;
        }
        std::cout << "Successfully inserted key:" << key << ", value:" << value << std::endl;
        _element_count++;//元素总数++
    }
    mtx.unlock();
    return 0;
}

// 打印跳表中的所有数据
template<typename K, typename V> 
void SkipList<K, V>::display_list() {

    std::cout << "\n*****Skip List*****"<<"\n"; 
    for (int i = 0; i <= _skip_list_level; i++) {
        Node<K, V> *node = this->_header->forward[i]; 
        std::cout << "Level " << i << ": ";
        while (node != NULL) {
            std::cout << node->get_key() << ":" << node->get_value() << ";";
            node = node->forward[i];
        }
        std::cout << std::endl;
    }
}

// 将数据从内存写入文件
template<typename K, typename V> 
void SkipList<K, V>::dump_file() {

    std::cout << "dump_file-----------------" << std::endl;
    _file_writer.open(STORE_FILE);
    Node<K, V> *node = this->_header->forward[0]; 

    //遍历，在第一层中取
    while (node != NULL) {
        _file_writer << node->get_key() << ":" << node->get_value() << "\n";
        std::cout << node->get_key() << ":" << node->get_value() << ";\n";
        node = node->forward[0];
    }

    _file_writer.flush();
    _file_writer.close();
    return ;
}

// 从磁盘中加载数据
template<typename K, typename V> 
void SkipList<K, V>::load_file() {

    _file_reader.open(STORE_FILE);
    std::cout << "load_file-----------------" << std::endl;
    std::string line;
    std::string* key = new std::string();
    std::string* value = new std::string();
    while (getline(_file_reader, line)) {
        get_key_value_from_string(line, key, value);
        if (key->empty() || value->empty()) {
            continue;
        }
        insert_element(*key, *value);
        std::cout << "key:" << *key << "value:" << *value << std::endl;
    }
    _file_reader.close();
}

// 拿到当前跳表的节点个数
template<typename K, typename V> 
int SkipList<K, V>::size() { 
    return _element_count;
}

template<typename K, typename V>
void SkipList<K, V>::get_key_value_from_string(const std::string& str, std::string* key, std::string* value) {

    if(!is_valid_string(str)) {
        return;
    }
    *key = str.substr(0, str.find(delimiter));
    *value = str.substr(str.find(delimiter)+1, str.length());
}

template<typename K, typename V>
bool SkipList<K, V>::is_valid_string(const std::string& str) {

    if (str.empty()) {
        return false;
    }
    if (str.find(delimiter) == std::string::npos) {
        return false;
    }
    return true;
}

// 删除跳表中的元素
template<typename K, typename V> 
void SkipList<K, V>::delete_element(K key) {

    mtx.lock();
    Node<K, V> *current = this->_header; 
    Node<K, V> *update[_max_level+1];
    memset(update, 0, sizeof(Node<K, V>*)*(_max_level+1));

    // start from highest level of skip list
    for (int i = _skip_list_level; i >= 0; i--) {
        while (current->forward[i] !=NULL && current->forward[i]->get_key() < key) {
            current = current->forward[i];
        }
        update[i] = current;
    }

    current = current->forward[0];
    if (current != NULL && current->get_key() == key) {
       
        // start for lowest level and delete the current node of each level
        for (int i = 0; i <= _skip_list_level; i++) {

            // if at level i, next node is not target node, break the loop.
            if (update[i]->forward[i] != current) 
                break;

            update[i]->forward[i] = current->forward[i];
        }

        // Remove levels which have no elements
        while (_skip_list_level > 0 && _header->forward[_skip_list_level] == 0) {
            _skip_list_level --; 
        }

        std::cout << "Successfully deleted key "<< key << std::endl;
        _element_count --;
    }
    mtx.unlock();
    return;
}

// Search for element in skip list 
/*
                           +------------+
                           |  select 60 |
                           +------------+
level 4     +-->1+                                                      100
                 |
                 |
level 3         1+-------->10+------------------>50+           70       100
                                                   |
                                                   |
level 2         1          10         30         50|           70       100
                                                   |
                                                   |
level 1         1    4     10         30         50|           70       100
                                                   |
                                                   |
level 0         1    4   9 10         30   40    50+-->60      70       100
*/

//在跳表中搜索元素
template<typename K, typename V> 
bool SkipList<K, V>::search_element(K key) {

    std::cout << "search_element-----------------" << std::endl;
    Node<K, V> *current = _header;//拿到头节点

    // 从跳表的最高层开始
    for (int i = _skip_list_level; i >= 0; i--) {
        //同插入元素中的过程
        while (current->forward[i] != nullptr && current->forward[i]->get_key() < key) {
            current = current->forward[i];
        }
    }

    //此时current为我们要搜索的结点
    current = current->forward[0];

    // 验证键值是否是我们要的
    if (current and current->get_key() == key) {
        std::cout << "Found key: " << key << ", value: " << current->get_value() << std::endl;
        return true;
    }

    std::cout << "Not Found Key:" << key << std::endl;
    return false;
}

// 跳表的构造函数
template<typename K, typename V> 
SkipList<K, V>::SkipList(int max_level) {

    this->_max_level = max_level;
    this->_skip_list_level = 0;
    this->_element_count = 0;

    // 创建头节点，并且指定k v都为NULL。
    K k;
    V v;
    this->_header = new Node<K, V>(k, v, _max_level);
};

// 跳表的析构函数
template<typename K, typename V> 
SkipList<K, V>::~SkipList() {

    if (_file_writer.is_open()) {
        _file_writer.close();
    }
    if (_file_reader.is_open()) {
        _file_reader.close();
    }
    delete _header;
}

// 生成随机层数
template<typename K, typename V>
int SkipList<K, V>::get_random_level(){

    int k = 1;
    while (rand() % 2) {
        k++;
    }
    k = (k < _max_level) ? k : _max_level;
    return k;
};
// vim: et tw=100 ts=4 sw=4 cc=120
