/*
 * Created by MajesticWaffle on 7/26/22.
 * Copyright (c) 2022 Thicc Industries. All rights reserved.
 * This software is under the: Thicc-Industries-Do-What-You-Want-I-Dont-Care License.
 */

/*
 * NTN - Named Tree Node
 * Basically a copy of Minecraft's NBT but with nesting instead of lists, and less stuff that I think to be unnecessary.
 * This will probably be separated from Geode engine for use as a separate library eventually. //TODO I guess.
 *
 * File structure
 * -----------------------------------------------------------------------------
 * NTN Files are started with a ROOT node with name matching file name, not
 * including file extension.
 *
 * ROOT nodes may also be used to contain structures who's parent does not need to
 * contain a value.
 *
 * The beginning of one node is placed immediately after the previous node.
 * Nodes are presumed to be children of the previous node. TERM nodes
 * terminate branches, and must be placed between nodes at the same level,
 * and at the end of a file.
 *
 * Example file tree (Example.ntn)
 * -----------------------------------------------------------------------------
 * Example : ROOT
 *      ∟ Byte : BYTE
 *        Integer : INT
 *        Double : DOUBLE
 *        Structure : ROOT
 *              ∟ Bytes : BArray
 *                Ints : IArray
 *                Doubles : BArray
 *
 * For this tree structure, TERM nodes would be placed like this:
 * Example : ROOT
 *      ∟ Byte : BYTE
 *          ∟ TERM
 *        Integer : INT
 *          ∟ TERM
 *        Double : DOUBLE
 *          ∟ TERM
 *        Structure : ROOT
 *              ∟ Bytes : BArray
 *                  ∟ TERM
 *                Ints : IArray
 *                  ∟ TERM
 *                Doubles : BArray
 *                  ∟ TERM
 *                TERM
 *        TERM
 *
 * Nodes are saved in binary in the order they appear on the tree.
 *
 * Root node
 * -----------------------------------------------------------------------------
 * 0x00 - 0x03: Node type
 * 0x04 - 0x0F: reserved
 * 0x10 - 0x2F: Name (Null padded) '/' is forbidden as it is used as a separator
 *
 * Byte, Int, Double
 * -----------------------------------------------------------------------------
 * 0x00 - 0x03: Node type
 * 0x04 - 0x0F: reserved
 * 0x10 - 0x2F: Name (Null padded) '/' is forbidden as it is used as a separator
 * 0x30 - 0x3F: Data. Null padded to 16 bytes.
 *
 * BArray, IArray, DArray
 * -----------------------------------------------------------------------------
 * 0x00 - 0x03: Node type
 * 0x04 - 0x0F: reserved
 * 0x10 - 0x2F: Name (Null padded) '/' is forbidden as it is used as a separator
 * 0x30 - 0x33: Number of array elements
 * 0x34 - 0x__: Data
 *
 * String
 * -----------------------------------------------------------------------------
 * 0x00 - 0x03: Node type
 * 0x04 - 0x0F: reserved
 * 0x10 - 0x2F: Name (Null padded) '/' is forbidden as it is used as a separator
 * 0x30 - 0x__: Null terminated string
 *
 * Term node
 * -----------------------------------------------------------------------------
 * 0x00 - 0x03: Node type
 * 0x04 - 0x0F: reserved
 */

#include <cstring>
#include "iostream"
#include "filesystem"
#include "geode.h"
#include "bzlib.h"

enum NTN_Type{
    NTN_Root,       //File root, No data, Name equal to file name.
    NTN_Byte,       //A single byte
    NTN_Int,        //Signed int
    NTN_Double,     //Signed double
    NTN_BArray,     //Multiple bytes, first 4 bytes denotes size
    NTN_IArray,     //Array of signed int, first 4 bytes denotes size
    NTN_DArray,     //Array of signed doubles,  first 4 bytes denotes size
    NTN_String,     //Null terminated string
    NTN_Term        //Terminates tree branch / file. No data, no name
};

//Prevent wrong things from happening
typedef void NTN_File;

typedef struct NTN_Node{
    NTN_Type        type;
    const char*     name; //32-character limit (including null termination)
    unsigned char*  data; //Pointer to start of data section
} NTN_Node;

typedef struct NTNByteArr{
    int count;
    unsigned char* arr;
} NTNByteArr;

typedef struct NTNIntArr{
    int count;
    int* arr;
} NTNIntArr;

typedef struct NTNDoubleArr{
    int count;
    double* arr;
} NTNDoubleArr;

void ntn_print_tree(NTN_File* root);
NTN_File* ntn_add(NTN_File* p, NTN_Type type, const std::string& name, void* data);
NTN_File* ntn_open_file(Map* m, const std::string& filename);
void ntn_close_file(NTN_File* f);
NTN_File* ntn_create_file(const std::string& name);
void ntn_write_file(Map* map, NTN_File* file);

//TODO: Theres got to be a better way!
unsigned char   ntn_get_byte(NTN_File* file, const std::string& path);
int             ntn_get_int(NTN_File* file, const std::string& path);
double          ntn_get_double(NTN_File* file, const std::string& path);
NTNByteArr      ntn_get_barray(NTN_File* file, const std::string& path);
NTNIntArr       ntn_get_iarray(NTN_File* file, const std::string& path);
NTNDoubleArr    ntn_get_darray(NTN_File* file, const std::string& path);
const char*     ntn_get_string(NTN_File* file, const std::string& path);

int ntn_set_byte(NTN_File* file, const std::string& path, uchar data);
int ntn_set_int(NTN_File* file, const std::string& path, int data);
int ntn_set_double(NTN_File* file, const std::string& path, double data);
int ntn_set_barray(NTN_File* file, const std::string& path, uchar* array, uint len);
int ntn_set_iarray(NTN_File* file, const std::string& path, int* array, uint len);
int ntn_set_darray(NTN_File* file, const std::string& path, double* array, uint len);
int ntn_set_string(NTN_File* file, const std::string& path, std::string data);