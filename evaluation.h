typedef struct FileContent
{
    FILE* fp;
    const char* location;
    unsigned char c;
    unsigned char byte_seek;
    unsigned char* content;
    size_t file_size;
    size_t max_size;
    size_t temp_seek;
} FileContent;

typedef struct Evaluation
{
    unsigned int nb_distinct_char;
    unsigned int i;
    size_t* char_count;
    struct Node** sort_char;
} Evaluation;
