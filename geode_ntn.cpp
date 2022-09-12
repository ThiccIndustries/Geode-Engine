/*
 * Created by MajesticWaffle on 7/26/22.
 * Copyright (c) 2022 Thicc Industries. All rights reserved.
 * This software is under the: Thicc-Industries-Do-What-You-Want-I-Dont-Care License.
 */

#include "geode_ntn.h"

unsigned int ntn_get_data_size(NTN_Node* node){
    //Static sized nodes
    switch(node -> type){
        case NTN_Root:
        case NTN_Term:
            return 0;

        case NTN_Byte:
        case NTN_Int:
        case NTN_Double:
            return 16;
    }

    //Array-type nodes
    if(node -> type != NTN_String)
        return (*(unsigned int*)node -> data / 16) + 16;

    //String node
    unsigned int size;
    for(size = 0; node -> data[size] != '\0'; ++size);
    return ((size / 16) * 16) + 16; //Dont. Touch. It.
}

//Internal only
NTN_Node* ntn_get_node(unsigned char* node_data){
    NTN_Node* node = new NTN_Node;
    node -> type = *(NTN_Type*)node_data;
    node -> name = (const char*)&node_data[0x10];

    if(node -> type == NTN_Term)
        node -> data = &node_data[0x10];
    else
        node -> data = &node_data[0x30];

    return node;
}

void ntn_print_tree(NTN_File* root){
    unsigned char* adr;
    NTN_Node* adr_node = (NTN_Node*)root;

    int indent = 0;

    //Don't ask.
    while(!(indent == 1 && adr_node -> type == NTN_Term)) {

        if(adr_node -> type != NTN_Term) {
            for (unsigned int i = 0; i < indent; ++i)
                std::cout << "    ";

            std::cout << adr_node->type << " : " << adr_node->name << std::endl;
        }
        if(adr_node -> type == NTN_Term)
            indent-=1;
        else
            indent++;

        adr = &adr_node -> data[ntn_get_data_size(adr_node)];

        if(adr_node != root)
            delete adr_node;

        adr_node = ntn_get_node(adr);
    }
}

unsigned int ntn_tree_size(NTN_File* r){
    NTN_Node* root = (NTN_Node*)r;

    void* adr;

    NTN_Node* adr_node = root;

    int indent = 0;
    int running_size = 0;
    //Don't ask.

    while(!(indent == 1 && adr_node -> type == NTN_Term)) {

        if(adr_node -> type == NTN_Term)
            running_size += 0x10;
        else
            running_size += 0x30;

        running_size += ntn_get_data_size(adr_node);

        if(adr_node -> type == NTN_Term)
            indent-=1;
        else
            indent++;

        adr = &adr_node -> data[ntn_get_data_size(adr_node)];

        if(adr_node != root)
            delete adr_node;

        adr_node = ntn_get_node((unsigned char*)adr);

    }

    return running_size + 0x10;
}

//creates an orphaned node
NTN_Node* ntn_create_node(NTN_Type type, const char* name, void* data){
    NTN_Node* node = new NTN_Node;
    node -> type = type;
    node -> name = name;

    unsigned int data_size = ntn_get_data_size(node) + 0x10;
    node -> data = new unsigned char[data_size];
    for(int i = 0; i < data_size; ++i)
        node -> data[i] = 0x00;

    memcpy(&node -> data[0], data, data_size - 0x10);
    *(int*)&node -> data[data_size - 0x10] = NTN_Term;

    unsigned char* raw_node_1 = new unsigned char[data_size + 0x30];
    for(int i = 0; i < data_size + 0x30; ++i)
        raw_node_1[i] = 0x00;

    *(int*)raw_node_1 = node -> type;                      //Node type
    memcpy(&raw_node_1[0x10], node -> name, strlen(name) + 1);         //Node name
    memcpy(&raw_node_1[0x30], node -> data, data_size); //Remaining data

    return node;
}

NTN_Node* ntn_get_internal(NTN_File* node, const char* name){
    unsigned char* adr;
    NTN_Node* adr_node = (NTN_Node*)node;
    int indent = 0;

    while(!(indent == 1 && adr_node -> type == NTN_Term)) {
        if(strcmp(adr_node -> name, name) == 0 && indent == 1)
            return adr_node;

        if(adr_node -> type == NTN_Term)
            indent-=1;
        else
            indent++;

        adr = &adr_node -> data[ntn_get_data_size(adr_node)];
        if(adr_node != node)
            delete adr_node;

        adr_node = ntn_get_node(adr);
    }

    return nullptr;
}

NTN_Node* ntn_get(NTN_File* node, const std::string& name){
    int split = name.find_first_of('/');

    //need to recurse
    if(split != -1) {
        std::string first_node = name.substr(0, split);
        std::string rem_nodes = name.substr(split + 1);
        return ntn_get(ntn_get_internal(node, first_node.c_str()), rem_nodes);
    }else{
        return ntn_get_internal(node, name.c_str());
    }
}

NTN_File* ntn_add(NTN_File* p, NTN_Type type, const std::string& name, void* data) {

    //Get node name & parent address

    NTN_Node *root = (NTN_Node *) p;
    NTN_Node *parent;

    std::string node_name = name;

    int split = name.find_last_of('/');
    if (split != -1) {
        node_name = name.substr(split + 1);
        parent = ntn_get(p, name.substr(0, split));
    } else {
        parent = (NTN_Node *) p;
    }

    //Insertion point TODO: This will cause problems if root isn't formatted perfectly
    void *root_insertion_point = root->data + ntn_tree_size(root) - 0x40;
    void *insertion_point = parent->data + ntn_tree_size(parent) - 0x40;

    unsigned int insertion_offset = (char *) root_insertion_point - (char *) insertion_point + 0x10;


    NTN_Node *c = ntn_create_node(type, node_name.c_str(), data);
    unsigned int size1 = ntn_tree_size((NTN_Node *) root);
    unsigned int size2 = ntn_tree_size(c);

    unsigned char *raw_node_1 = new unsigned char[size1];     //Node type
    for (int i = 0; i < size1; ++i)
        raw_node_1[i] = 0x00;

    memcpy(&raw_node_1[0x10], root->name, strlen(root -> name) + 1);//Node name
    memcpy(&raw_node_1[0x30], root->data, size1 - 0x30);            //Remaining data

    unsigned char *raw_node_2 = new unsigned char[size2];
    for (int i = 0; i < size2; ++i)
        raw_node_2[i] = 0x00;

    *(int *) raw_node_2 = c->type;                              //Node type
    memcpy(&raw_node_2[0x10], c->name,  strlen(c -> name) + 1); //Node name
    memcpy(&raw_node_2[0x30], c->data, size2 - 0x30);           //Remaining data

    //Insertion point will be 16 bytes before end of parent (before final NTN_Term)
    unsigned char *new_memory_space = new unsigned char[size1 + size2];

    memcpy(new_memory_space, raw_node_1, size1 - insertion_offset);                     //Copy until intertion point
    memcpy(&new_memory_space[size1 - insertion_offset], raw_node_2, size2);                                        //Insert second node
    memcpy(&new_memory_space[size1 - insertion_offset + size2], &raw_node_1[size1 - insertion_offset], insertion_offset); //Copy NTN_Term

    delete[] root;
    delete[] c;
    delete[] raw_node_1;
    delete[] raw_node_2;

    return ntn_get_node(new_memory_space);
}

//This *will* cause a crash if given improper data.
NTN_File* ntn_open_file(Map* m, const std::string& filename){
    int size = world_get_resource_size(m, filename + ".ntn");

    if(size == -1)
        return nullptr;

    unsigned char* buffer = new unsigned char[size];

    if(world_read_map_resource(m, filename +".ntn", buffer) == -1)
        error("Invalid NTN File", "Unable to read NTN file: " + filename);

    NTN_Node* node = ntn_get_node(buffer);

    if(node -> type != NTN_Root) {
        delete[] buffer;
        return nullptr;
    }

    return node;
}

void ntn_close_file(NTN_File* f){
    NTN_Node* node = (NTN_Node*)f;

    delete node -> name;
    delete node -> data;
    delete node;
}

void ntn_write_file(Map* m, NTN_File* file){
    NTN_Node* file_node = (NTN_Node*)file;
    int size = ntn_tree_size(file);
    unsigned char* file_raw = new unsigned char[size];

    for(int i = 0; i < size; ++i)
        file_raw[i] = 0x00;

    *(int*)&file_raw[0] = file_node -> type;
    memcpy(&file_raw[0x10], file_node -> name, strlen(file_node->name) + 1);
    memcpy(&file_raw[0x30], file_node -> data, size - 0x30);

    world_write_map_resource(m, std::string(file_node -> name) + ".ntn", file_raw, size);
    delete[] file_raw;
}

NTN_File* ntn_create_file(const std::string& name){
    unsigned char* data = new unsigned char[0x40];
    for(int i = 0; i < 0x40; ++i)
        data[i] = 0x00;

    *(int*)&data[0] = NTN_Root;
    memcpy(&data[0x10], name.c_str(), strlen(name.c_str()) + 1);
    *(int*)&data[0x30] = NTN_Term;

    NTN_Node* node = new NTN_Node;
    node -> type = NTN_Root;
    node -> name = strdup(name.c_str());

    node -> data = &data[0x30];

    return node;
}

//TODO: Theres got to be a better way!
unsigned char ntn_get_byte(NTN_File* file, const std::string& path){
    NTN_Node* node = ntn_get(file, path);
    if(node -> type != NTN_Byte){
        error("NTN type mismatch", "Node: " + path + "accessed as byte.");
        return 0;
    }
    return *(unsigned char*)node -> data;
}
int ntn_get_int(NTN_File* file, const std::string& path){
    NTN_Node* node = ntn_get(file, path);

    if(node == nullptr){
        error("Nonexistent NTN node", "Node: " + path + "does not exist in file: " + ((NTN_Node*)file) -> name);
        return 0;
    }

    if(node -> type != NTN_Int){
        error("NTN node type mismatch", "Node: " + path + "accessed as int. in file: " + ((NTN_Node*)file) -> name);
        return 0;
    }

    return *(int*)node -> data;
}
double ntn_get_double(NTN_File* file, const std::string& path){
    NTN_Node* node = ntn_get(file, path);
    if(node == nullptr){
        error("Nonexistent NTN node", "Node: " + path + "does not exist in file: " + ((NTN_Node*)file) -> name);
        return 0;
    }

    if(node -> type != NTN_Int){
        error("NTN node type mismatch", "Node: " + path + "accessed as double. in file: " + ((NTN_Node*)file) -> name);
        return 0;
    }
    return *(double*)node -> data;
}
NTNByteArr ntn_get_barray(NTN_File* file, const std::string& path){
    NTN_Node* node = ntn_get(file, path);
    if(node == nullptr){
        error("Nonexistent NTN node", "Node: " + path + "does not exist in file: " + ((NTN_Node*)file) -> name);
        return {0, nullptr};
    }
    if(node -> type != NTN_BArray){
        error("NTN node type mismatch", "Node: " + path + "accessed as byte array. in file: " + ((NTN_Node*)file) -> name);
        return {0, nullptr};
    }

    return {(int)node -> data[0], (unsigned char*)&node -> data[4]};
}
NTNIntArr ntn_get_iarray(NTN_File* file, const std::string& path){
    NTN_Node* node = ntn_get(file, path);
    if(node == nullptr){
        error("Nonexistent NTN node", "Node: " + path + "does not exist in file: " + ((NTN_Node*)file) -> name);
        return {0, nullptr};
    }
    if(node -> type != NTN_IArray){
        error("NTN node type mismatch", "Node: " + path + "accessed as int array. in file: " + ((NTN_Node*)file) -> name);
        return {0, nullptr};
    }

    return {(int)node -> data[0], (int*)&node -> data[4]};
}
NTNDoubleArr ntn_get_darray(NTN_File* file, const std::string& path){
    NTN_Node* node = ntn_get(file, path);
    if(node == nullptr){
        error("Nonexistent NTN node", "Node: " + path + "does not exist in file: " + ((NTN_Node*)file) -> name);
        return {0, nullptr};
    }
    if(node -> type != NTN_DArray){
        error("NTN node type mismatch", "Node: " + path + "accessed as double array. in file: " + ((NTN_Node*)file) -> name);
        return {0, nullptr};
    }

    return {(int)node -> data[0], (double*)&node -> data[4]};
}
const char* ntn_get_string(NTN_File* file, const std::string& path){
    NTN_Node* node = ntn_get(file, path);
    if(node == nullptr){
        error("Nonexistent NTN node", "Node: " + path + "does not exist in file: " + ((NTN_Node*)file) -> name);
        return nullptr;
    }
    if(node -> type != NTN_Int){
        error("NTN node type mismatch", "Node: " + path + "accessed as string. in file: " + ((NTN_Node*)file) -> name);
        return nullptr;
    }
    return (const char*)node -> data;
}

int ntn_set_byte(NTN_File* file, const std::string& path, uchar data){
    NTN_Node* node = ntn_get(file, path);
    if(node == nullptr)
        return -1;

    *(uchar*)&node -> data[0] = data;
    return 1;
}
int ntn_set_int(NTN_File* file, const std::string& path, int data){
    NTN_Node* node = ntn_get(file, path);
    if(node == nullptr)
        return -1;

    *(int*)&node -> data[0] = data;
    return 1;
}
int ntn_set_double(NTN_File* file, const std::string& path, double data){
    NTN_Node* node = ntn_get(file, path);
    if(node == nullptr)
        return -1;

    *(double*)&node -> data[0] = data;
    return 1;
}

int ntn_set_barray(NTN_File* file, const std::string& path, uchar* array, uint len);
int ntn_set_iarray(NTN_File* file, const std::string& path, int* array, uint len);
int ntn_set_darray(NTN_File* file, const std::string& path, double* array, uint len);
int ntn_set_string(NTN_File* file, const std::string& path, std::string data);