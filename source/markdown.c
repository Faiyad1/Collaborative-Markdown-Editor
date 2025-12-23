#include "../libs/markdown.h"

#include <stdlib.h>
#include <string.h>


static int insert_in_two_place(
    document *doc, uint64_t start, uint64_t end, uint64_t start_len, 
    uint64_t end_len, char *start_con, char *end_con);
    
static void insert_in_one_place(document *doc, uint64_t pos, uint64_t len, char *content);
static void delete(document *doc, uint64_t pos, uint64_t len);
static chunk* create_chunk(uint64_t len, char *content, int is_new_data, int is_deleted);

// Assinging default values to respective functions
#define create_new_chunk(a, b) create_chunk(a, b, 1, 0)
#define copy_chunk(a, b, d) create_chunk(a, b, 0, d)
#define delete_chunk(a, b) create_chunk(a, b, 0, 1)


#define SUCCESS 0 

// === Init and Free ===
document *markdown_init(void) {
    document* doc = malloc(sizeof(document));
    doc->version = 0;
    doc->data = NULL;
    doc->no_of_char = 0;
    doc->starting_chunk = NULL;

    return doc;
}

void markdown_free(document *doc) {

    chunk* current_chunk = doc->starting_chunk;
    chunk* previous_chunk = NULL;
    
    while (current_chunk != NULL){
        previous_chunk = current_chunk;
        current_chunk = current_chunk->next_chunk;

        if (previous_chunk != NULL){
            free(previous_chunk->data);    
            free(previous_chunk);
        }
    }

    if (doc->data != NULL){
        free(doc->data); 
    }
    free(doc);
}

// === Edit Commands ===
int markdown_insert(document *doc, uint64_t version, size_t pos, const char *content) {
    if (version != doc->version){
        return -3;
    }
    if (pos > doc->no_of_char){
        return -1;
    }

    // Getting the size of the content
    int len = 0;
    for (len = 0; len >= 0; len++){
        if (content[len] == '\n'){
            //TO DO
            return -4;
            //
        } else if (content[len] == '\0'){
            break;
        }
    }

    char *mod_content = malloc(sizeof(char) * len);
    memcpy(mod_content, content, sizeof(char) * len);
    insert_in_one_place(doc, pos, len, mod_content);
    return SUCCESS;
}

int markdown_delete(document *doc, uint64_t version, size_t pos, size_t len) {
    if (version != doc->version){
        return -3;
    }
    if (pos + len > doc->no_of_char){
        return -1;
    }

    delete(doc, pos, len);
    return SUCCESS;
}

// === Formatting Commands ===
int markdown_newline(document *doc, uint64_t version, size_t pos) {
    if (version != doc->version){
        return -3;
    }
    if (pos > doc->no_of_char){
        return -1;
    }

    char *mod_content = malloc(sizeof(char) * 1);
    memcpy(mod_content, "\n", sizeof(char) * 1);
    insert_in_one_place(doc, pos, 1, mod_content);
    return SUCCESS;
}

int markdown_heading(document *doc, uint64_t version, int level, size_t pos) {
    if (version != doc->version){
        return -3;
    }
    if (pos > doc->no_of_char){
        return -1;
    }

    // Checking if it has new line before pos
    int inc = 0;
    if (pos != 0){
        if (doc->data[pos - 1] != '\n'){
            inc++;
        }
    }

    int len = level + 1 + inc;
    char *mod_content = malloc(sizeof(char) * len);

    if (inc){
        memcpy(mod_content, "\n", sizeof(char) * 1);
    }

    if (level == 1){
        memcpy(&mod_content[inc], "# ", sizeof(char) * level + 1);
    } else if (level == 2){
        memcpy(&mod_content[inc], "## ", sizeof(char) * level + 1);
    } else{
        memcpy(&mod_content[inc], "### ", sizeof(char) * level + 1);
    } 
    
    insert_in_one_place(doc, pos, len, mod_content);
    return SUCCESS;
}

int markdown_bold(document *doc, uint64_t version, size_t start, size_t end) {
    if (version != doc->version){
        return -3;
    }
    if (end > doc->no_of_char || end <= start){
        return -1;
    }

    char *mod_content = malloc(sizeof(char) * 2);
    memcpy(mod_content, "**", sizeof(char) * 2);
    
    char *mod_content1 = malloc(sizeof(char) * 2);
    memcpy(mod_content1, "**", sizeof(char) * 2);

    return insert_in_two_place(doc, start, end, 2, 2, mod_content, mod_content1);
}

int markdown_italic(document *doc, uint64_t version, size_t start, size_t end) {
    if (version != doc->version){
        return -3;
    }
    if (end > doc->no_of_char || end <= start){
        return -1;
    }

    char *mod_content = malloc(sizeof(char) * 1);
    memcpy(mod_content, "*", sizeof(char) * 1);
    
    char *mod_content1 = malloc(sizeof(char) * 1);
    memcpy(mod_content1, "*", sizeof(char) * 1);

    return insert_in_two_place(doc, start, end, 1, 1, mod_content, mod_content1);
}

int markdown_blockquote(document *doc, uint64_t version, size_t pos) {
    if (version != doc->version){
        return -3;
    }
    if (pos > doc->no_of_char){
        return -1;
    }

    // Checking if it has new line before pos
    int inc = 0;
    if (pos != 0){
        if (doc->data[pos - 1] != '\n'){
            inc++;
        }
    }

    int len = 2 + inc;
    char *mod_content = malloc(sizeof(char) * len);

    if (inc){
        memcpy(mod_content, "\n> ", sizeof(char) * len);
    } else{
        memcpy(mod_content, "> ", sizeof(char) * len);
    }

    insert_in_one_place(doc, pos, len, mod_content);
    return SUCCESS;
}

int markdown_ordered_list(document *doc, uint64_t version, size_t pos) {
    if (version != doc->version){
        return -3;
    }
    if (pos > doc->no_of_char){
        return -1;
    }

    // Checking if it has new line before pos
    int inc = 0;
    if (pos != 0){
        if (doc->data[pos - 1] != '\n'){
            inc++;
        }
    }

    int len = 3 + inc;
    char *mod_content = malloc(sizeof(char) * len);

    if (inc){
        memcpy(mod_content, "\n1. ", sizeof(char) * len);
    } else{
        memcpy(mod_content, "1. ", sizeof(char) * len);
    }

    insert_in_one_place(doc, pos, len, mod_content);
    return SUCCESS;
}

int markdown_unordered_list(document *doc, uint64_t version, size_t pos) {
    if (version != doc->version){
        return -3;
    }
    if (pos > doc->no_of_char){
        return -1;
    }

    // Checking if it has new line before pos
    int inc = 0;
    if (pos != 0){
        if (doc->data[pos - 1] != '\n'){
            inc++;
        }
    }

    int len = 2 + inc;
    char *mod_content = malloc(sizeof(char) * len);

    if (inc){
        memcpy(mod_content, "\n- ", sizeof(char) * len);
    } else{
        memcpy(mod_content, "- ", sizeof(char) * len);
    }

    insert_in_one_place(doc, pos, len, mod_content);
    return SUCCESS;
}

int markdown_code(document *doc, uint64_t version, size_t start, size_t end) {
    if (version != doc->version){
        return -3;
    }
    if (end > doc->no_of_char || end <= start){
        return -1;
    }


    char *mod_content = malloc(sizeof(char) * 1);
    memcpy(mod_content, "`", sizeof(char) * 1);
    
    char *mod_content1 = malloc(sizeof(char) * 1);
    memcpy(mod_content1, "`", sizeof(char) * 1);

    return insert_in_two_place(doc, start, end, 1, 1, mod_content, mod_content1);
}

int markdown_horizontal_rule(document *doc, uint64_t version, size_t pos) {
    if (version != doc->version){
        return -3;
    }
    if (pos > doc->no_of_char){
        return -1;
    }

    int inc = 0;
    int pre_nline = 1;
    int post_nline = 1;

    // Checking if it has new line before pos
    if (pos != 0){
        if (doc->data[pos - 1] != '\n'){
            inc++;
            pre_nline = 0;
        }
    }

    // Checking if it has new line after pos
    if (pos != doc->no_of_char){
        if (doc->data[pos] != '\n'){
            inc++;
            post_nline = 0;
        }
    } else{ // No existing data after pos
        inc++;
        post_nline = 0;
    }

    int len = 3 + inc;
    char *mod_content = malloc(sizeof(char) * len);

    if (pre_nline && post_nline){
        memcpy(mod_content, "---", sizeof(char) * len);
    } else if(pre_nline && !post_nline){
        memcpy(mod_content, "---\n", sizeof(char) * len);
    } else if(!pre_nline && post_nline){
        memcpy(mod_content, "\n---", sizeof(char) * len);
    } else{
        memcpy(mod_content, "\n---\n", sizeof(char) * len);
    }

    insert_in_one_place(doc, pos, len, mod_content);
    return SUCCESS;
}

int markdown_link(document *doc, uint64_t version, size_t start, size_t end, const char *url) {
    if (version != doc->version){
        return -3;
    }
    if (end > doc->no_of_char || end <= start){
        return -1;
    }

    // Getting the size of the url link
    int len = 0;
    for (len = 0; len >= 0; len++){
        if (url[len] == '\0'){
            break;
        }
    }
    
    char *mod_content = malloc(sizeof(char) * 1);
    memcpy(mod_content, "[", sizeof(char) * 1);
    
    char *mod_content1 = malloc(sizeof(char) * len + 3);
    memcpy(mod_content1, "](", sizeof(char) * 2);
    memcpy(&mod_content1[2], url, sizeof(char) * len);
    memcpy(&mod_content1[len + 2], ")", sizeof(char) * 1);

    return insert_in_two_place(doc, start, end, 1, len + 3, mod_content, mod_content1);
}

// === Utilities ===
void markdown_print(const document *doc, FILE *stream) {
    fwrite(doc->data, sizeof(char), doc->no_of_char, stream);
}

char *markdown_flatten(const document *doc) {
    char *data = malloc(sizeof(char) * (doc->no_of_char + 1));
    memcpy(data, doc->data, sizeof(char) * doc->no_of_char);
    memcpy(&data[doc->no_of_char], "\0", sizeof(char));
    return data;
}

// === Versioning ===
void markdown_increment_version(document *doc) {

    uint64_t cur_pos = 0;
    char *content = NULL;
    chunk* cnk = doc->starting_chunk;
    chunk* prev_cnk = cnk;

    while (cnk != NULL){

        if (!cnk->is_deleted){ //When the data is not being deleted

            //Updating the new inserted data into current data
            cnk->is_new_data = 0;
            cur_pos += cnk->no_of_char;
            content = realloc(content, sizeof(char) * cur_pos);
            memcpy(&content[cur_pos - cnk->no_of_char], cnk->data, sizeof(char) * cnk->no_of_char);

            prev_cnk = cnk;
            cnk = cnk->next_chunk;

        } else{
            if (cur_pos == 0){
                doc->starting_chunk = cnk->next_chunk;
            } else{
                prev_cnk->next_chunk = cnk->next_chunk;
            }

            free(cnk->data);
            free(cnk);

            if (cur_pos == 0){
                cnk = doc->starting_chunk;
            } else{
                cnk = prev_cnk->next_chunk;
            }
        }
    }


    // serealize ordered_lists numbers
    int list_no = 1;
    for (uint64_t len = 0; len + 3 < cur_pos; len++){
        if (len == 0
            && content[len] >= '1' && content[len] <= '9'
            && content[len + 1] == '.'
            && content[len + 2] == ' '){

            content[len] = '0' + list_no;
            list_no++;

        } else if (content[len] == '\n' 
            && content[len + 1] >= '1' && content[len + 1] <= '9'
            && content[len + 2] == '.'
            && content[len + 3] == ' '){

            content[len + 1] = '0' + list_no;
            list_no++;

        } else if (content[len] == '\n'){
            list_no = 1;
        }
    }


    //Update the current document
    if (doc->data != NULL){
        free(doc->data);
    }

    doc->data = content;
    doc->no_of_char = cur_pos;

    doc->version++;
}


// === Extra Private Functions ===
static int insert_in_two_place(
    document *doc, uint64_t start, uint64_t end, uint64_t start_len, 
    uint64_t end_len, char *start_con, char *end_con) {

    uint64_t cur_pos = 0;
    chunk* current_chunk = doc->starting_chunk;
    while (current_chunk != NULL){     // Checking for DELETED_POSITION
        if (!current_chunk->is_new_data){
            cur_pos += current_chunk->no_of_char;
        }

        if (cur_pos >= start && !current_chunk->is_deleted){
            break;
        }

        if (start >= end){
                return -3;
        }

        current_chunk = current_chunk->next_chunk;
    }

    insert_in_one_place(doc, start, start_len, start_con);
    insert_in_one_place(doc, end, end_len, end_con);

    return 0;
}


static void insert_in_one_place(document *doc, uint64_t pos, uint64_t len, char *content){

    if (doc->starting_chunk == NULL){
        chunk* cnk_ptr = create_new_chunk(len, content);
        doc->starting_chunk = cnk_ptr;

    } else if(pos == 0){     //Inserting at the start of the doc
        chunk* cnk_ptr = create_new_chunk(len, content);
        cnk_ptr->next_chunk = doc->starting_chunk;
        doc->starting_chunk = cnk_ptr;
        
    } else {
        uint64_t cur_pos = 0;
        chunk* current_chunk = doc->starting_chunk;
        chunk* previous_chunk = NULL;
        while (current_chunk != NULL){
            if (!current_chunk->is_new_data){
                cur_pos += current_chunk->no_of_char;
            }

            if (cur_pos >= pos){
                break;
            }

            previous_chunk = current_chunk;
            current_chunk = current_chunk->next_chunk;
        }


        // Splitting the chunk into 3 parts (pre_content, content, post_content)
        uint64_t pre_exists = 0;
        uint64_t pre_len = pos - (cur_pos - current_chunk->no_of_char);
        chunk* pre_cnk_ptr;
        if (pre_len){
            char *pre_content = malloc(sizeof(char) * pre_len);
            memcpy(pre_content, current_chunk->data, sizeof(char) * pre_len);
            pre_cnk_ptr = copy_chunk(pre_len, pre_content, current_chunk->is_deleted);
            pre_exists = 1;
        }

        chunk* cnk_ptr = create_new_chunk(len, content);

        uint64_t post_exists = 0;
        uint64_t post_len = cur_pos - pos;
        chunk* post_cnk_ptr;
        if (post_len){
            char *post_content = malloc(sizeof(char) * post_len);
            memcpy(post_content, &current_chunk->data[pre_len], sizeof(char) * post_len);
            post_cnk_ptr = copy_chunk(post_len, post_content, current_chunk->is_deleted);
            post_exists = 1;
        }

        
        // Rearranging and assigning them inside the linked list
        if (previous_chunk == NULL){ // If the splitted chunk is the starting chunk
            if (pre_exists){
                doc->starting_chunk = pre_cnk_ptr;
                pre_cnk_ptr->next_chunk = cnk_ptr;
            } else{
                doc->starting_chunk = cnk_ptr;
            }
        } else{
            if (pre_exists){
                previous_chunk->next_chunk = pre_cnk_ptr;
                pre_cnk_ptr->next_chunk = cnk_ptr;
            } else{
                previous_chunk->next_chunk = cnk_ptr;
            }
        }

        if (post_exists){
            cnk_ptr->next_chunk = post_cnk_ptr;
            post_cnk_ptr->next_chunk = current_chunk->next_chunk;
        } else{
            cnk_ptr->next_chunk = current_chunk->next_chunk;
        } 


        free(current_chunk->data);
        free(current_chunk);
    }
}


static void delete(document *doc, uint64_t pos, uint64_t len){

    // Working with starting position
    uint64_t cur_pos = 0;
    chunk* current_chunk = doc->starting_chunk;
    chunk* previous_chunk = NULL;
    while (current_chunk != NULL){
        if (!current_chunk->is_new_data){
            cur_pos += current_chunk->no_of_char;
        }

        if (cur_pos >= pos){
            break;
        }

        previous_chunk = current_chunk;
        current_chunk = current_chunk->next_chunk;
    }


    // Splitting the chunk into 2 parts (pre_content, post_content)
    uint64_t pre_exists = 0;
    uint64_t pre_len = pos - (cur_pos - current_chunk->no_of_char);
    chunk* pre_cnk_ptr;
    if (pre_len){
        char *pre_content = malloc(sizeof(char) * pre_len);
        memcpy(pre_content, current_chunk->data, sizeof(char) * pre_len);
        pre_cnk_ptr = copy_chunk(pre_len, pre_content, current_chunk->is_deleted);
            pre_exists = 1;
    }

    uint64_t post_exists = 0;
    uint64_t post_len = cur_pos - pos;
    chunk* post_cnk_ptr;
    if (post_len){
        char *post_content = malloc(sizeof(char) * post_len);
        memcpy(post_content, &current_chunk->data[pre_len], sizeof(char) * post_len);
        post_cnk_ptr = copy_chunk(post_len, post_content, current_chunk->is_deleted);
        post_exists = 1;
    }

        
    // Rearranging and assigning them inside the linked list
    if (previous_chunk == NULL){     // If the splitted chunk is the starting chunk
        if (pre_exists){
            doc->starting_chunk = pre_cnk_ptr;
            if (post_exists){
                pre_cnk_ptr->next_chunk = post_cnk_ptr;
                post_cnk_ptr->next_chunk = current_chunk->next_chunk;
            } else{
                pre_cnk_ptr->next_chunk = current_chunk->next_chunk;
            }
        } else{
            doc->starting_chunk = post_cnk_ptr;
            post_cnk_ptr->next_chunk = current_chunk->next_chunk;
        }

    } else{
        if (pre_exists && post_exists){
            previous_chunk->next_chunk = pre_cnk_ptr;
            pre_cnk_ptr->next_chunk = post_cnk_ptr;
            post_cnk_ptr->next_chunk = current_chunk->next_chunk;
        } else if (pre_exists){
            previous_chunk->next_chunk = pre_cnk_ptr;
            pre_cnk_ptr->next_chunk = current_chunk->next_chunk;
        } else{
            previous_chunk->next_chunk = post_cnk_ptr;
            post_cnk_ptr->next_chunk = current_chunk->next_chunk;
        }
    }

    free(current_chunk->data);
    free(current_chunk);


    // Working with ending position
    cur_pos = 0;
    if (pre_exists){
        previous_chunk = pre_cnk_ptr;
    } else{
        previous_chunk = NULL;
    }

    if (post_exists){
        current_chunk = post_cnk_ptr;
    } else{
        current_chunk = pre_cnk_ptr->next_chunk;
    }

    // Finding the chunk which contains the ending position
    while (current_chunk != NULL){
        if (!current_chunk->is_new_data){
            cur_pos += current_chunk->no_of_char;
        }

        if (cur_pos >= len){
            break;
        }

        if (!current_chunk->is_new_data){
            current_chunk->is_deleted = 1;
        }

        previous_chunk = current_chunk;
        current_chunk = current_chunk->next_chunk;
    }


    // Splitting the chunk into 2 parts (pre_content, post_content)
    pre_exists = 0;
    pre_len = len - (cur_pos - current_chunk->no_of_char);
    if (pre_len){
        char *pre_content = malloc(sizeof(char) * pre_len);
        memcpy(pre_content, current_chunk->data, sizeof(char) * pre_len);
        pre_cnk_ptr = delete_chunk(pre_len, pre_content);
        pre_exists = 1;

        pre_cnk_ptr->is_deleted = 1;
    }

    post_exists = 0;
    post_len = cur_pos - len;
    if (post_len){
        char *post_content = malloc(sizeof(char) * post_len);
        memcpy(post_content, &current_chunk->data[pre_len], sizeof(char) * post_len);
        post_cnk_ptr = copy_chunk(post_len, post_content, current_chunk->is_deleted);
        post_exists = 1;
    }

        
    // Assigning inside the linked list
    if (previous_chunk == NULL){     // If the splitted chunk is the starting chunk
        doc->starting_chunk = pre_cnk_ptr;
        if (post_exists){
            pre_cnk_ptr->next_chunk = post_cnk_ptr;
            post_cnk_ptr->next_chunk = current_chunk->next_chunk;
        } else{
            pre_cnk_ptr->next_chunk = current_chunk->next_chunk;
        }

    } else{
        if (pre_exists && post_exists){
            previous_chunk->next_chunk = pre_cnk_ptr;
            pre_cnk_ptr->next_chunk = post_cnk_ptr;
            post_cnk_ptr->next_chunk = current_chunk->next_chunk;
        } else if (pre_exists){
            previous_chunk->next_chunk = pre_cnk_ptr;
            pre_cnk_ptr->next_chunk = current_chunk->next_chunk;
        } else{
            previous_chunk->next_chunk = post_cnk_ptr;
            post_cnk_ptr->next_chunk = current_chunk->next_chunk;
        }
    }

    free(current_chunk->data);
    free(current_chunk);
}


static chunk* create_chunk(uint64_t len, char *content, int is_new_data, int is_deleted){
    chunk* cnk_ptr = malloc(sizeof(chunk));
    cnk_ptr->data = content;
    cnk_ptr->no_of_char = len;

    cnk_ptr->is_new_data = is_new_data;
    cnk_ptr->is_deleted = is_deleted;
    cnk_ptr->next_chunk = NULL;
    return cnk_ptr;
}