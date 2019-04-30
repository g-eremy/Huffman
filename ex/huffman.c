#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "./arbre.h"
#include "./evaluation.h"

#define SIZEBLOCK 134217728
//#define SIZEBLOCK 1024

FileContent* getFileContent(const char* location, bool write, size_t max_size)
{
    FileContent *f = malloc(sizeof(FileContent));
    f->fp = fopen(location, write ? "w+" : "r");
    f->location = location;
    f->temp_seek = 0;
    f->byte_seek = 128;

    if (!f->fp && !write)
    {
    	f->content = NULL;
    	f->file_size = 0;
   
        return f;
    }

    if (!write)
    {
        fseek(f->fp, 0, SEEK_END);
        f->file_size = ftell(f->fp);
        fseek(f->fp, 0, SEEK_SET);
        max_size = (f->file_size > SIZEBLOCK) ? SIZEBLOCK : f->file_size;
        f->content = malloc(max_size);
        fread(f->content, sizeof(unsigned char), max_size, f->fp);
    }
    else
    {
        f->content = (max_size > SIZEBLOCK || max_size == 0) ? malloc(SIZEBLOCK * sizeof(unsigned char)) : malloc(max_size * sizeof(unsigned char));
        f->c = 0;
        f->file_size = 1;
        f->max_size = max_size;
    }

    return f;
}

bool fileExist(const char* location)
{
    FILE* fp = fopen(location, "r");

    if (fp != NULL)
    {
        fclose(fp);
        return true;
    }

    return false;
}

void nextBlock(FileContent* f, bool write, size_t i)
{
    size_t r = (!write) ? f->file_size - i : f->max_size - f->file_size;
    size_t max_size = (r > SIZEBLOCK || max_size == 0) ? SIZEBLOCK : r;

    if (f->temp_seek >= (SIZEBLOCK - 1) && f->content != NULL)
    {
        if (write)
        {
            fseek(f->fp, 0, SEEK_END);
            fwrite(f->content, sizeof(unsigned char), f->temp_seek, f->fp);
        }

        free(f->content);
        f->content = malloc(max_size * sizeof(unsigned char));

        if (!write)
        {
            fseek(f->fp, i, SEEK_SET);
            fread(f->content, sizeof(unsigned char), max_size, f->fp);
        }

        f->temp_seek = 0;
    }
}

void firstBlock(FileContent* f)
{
    size_t max_size = (f->file_size > SIZEBLOCK) ? SIZEBLOCK : f->file_size;
    f->temp_seek = 0;

    if (max_size != f->file_size)
    {
        free(f->content);
        f->content = malloc(max_size);
        fseek(f->fp, 0, SEEK_SET);
        fread(f->content, sizeof(unsigned char), max_size, f->fp);
    }
}

void addBit(FileContent* f, bool bit)
{
    if (bit)
    {
        f->c += f->byte_seek;
    }

    if (f->byte_seek != 1)
    {
        f->byte_seek /= 2;
    }
    else
    {
        f->content[f->temp_seek] = f->c;
        f->file_size++;
        f->temp_seek++;
        f->c = 0;
        f->byte_seek = 128;
        nextBlock(f, true, 0);
    }
}

bool getBit(FileContent* f, size_t* i)
{
    bool bit = ((f->content[f->temp_seek] & f->byte_seek) == f->byte_seek) ? true : false;

    if (f->byte_seek != 1)
    {
        f->byte_seek /= 2;
    }
    else
    {
        *i += 1;
        f->temp_seek++;
        f->byte_seek = 128;
        nextBlock(f, false, *i);
    }

    return bit;
}

bool compressionEnd(FileContent* f, size_t i, unsigned char ignore)
{
    return (i < (f->file_size - 1) || (i == (f->file_size - 1) && ignore < f->byte_seek));
}

Node* newLeaf(unsigned char value, size_t occurence)
{
    Node* leaf = malloc(sizeof(Node));
    leaf->left = NULL;
    leaf->right = NULL;
    leaf->is_node = false;
    leaf->occurence = occurence;
    leaf->value = value;

    return leaf;
}

Evaluation* evaluate(FileContent* f)
{
    Evaluation* e = malloc(sizeof(Evaluation));
    e->nb_distinct_char = 0;
    e->char_count = malloc(256 * sizeof(size_t));
    e->i = 0;
    unsigned int i;

    for (i = 0; i < 256; i++)
    {
        e->char_count[i] = 0;
    }

    for (size_t j = 0; j < f->file_size; j++)
    {
    	nextBlock(f, false, j);
  
        if (f->content[f->temp_seek] >= 256 || f->content[f->temp_seek] < 0)
        {
            return NULL;
        }
        else
        {
            if (e->char_count[f->content[f->temp_seek]] == 0) e->nb_distinct_char++;
            e->char_count[f->content[f->temp_seek]]++;
        }

        f->temp_seek++;
    }

    firstBlock(f);
    size_t* library = malloc(256 * sizeof(size_t));
    memcpy(library, e->char_count, (256 * sizeof(size_t)));
    e->sort_char = malloc(e->nb_distinct_char * sizeof(Node));
    i = 0;
    unsigned char min = 0;
    unsigned char begin = 0;

    do
    {
        min = begin;

        if (library[begin] != 0)
        {
            for (unsigned int j = 0; j < 256; j++)
            {
                if (library[j] != 0 && library[j] < library[min])
                {
                    min = j;
                }
            }

            e->sort_char[i] = newLeaf(min, e->char_count[min]);
            library[min] = 0;
            i++;
        }
        else
        {
            begin++;
        }
    } while(i < e->nb_distinct_char && begin < 256);

    free(library);

    return e;
}

Tree* newTree()
{
    Tree *tree = malloc(sizeof(Tree));
    tree->root = NULL;
    tree->node_search = malloc(sizeof(Node));
    tree->leafs = malloc(256 * sizeof(Node));
    tree->next_node = 0;

    return tree;
}

void classTree(Evaluation* e, Node* node)
{
    e->sort_char[e->i] = NULL;
    e->i++;
    e->sort_char[e->i] = node;
    unsigned int i = e->i + 1;
    Node *temp;

    while (i < e->nb_distinct_char && node->occurence > e->sort_char[i]->occurence)
    {
        temp = e->sort_char[i];
        e->sort_char[i] = node;
        e->sort_char[i - 1] = temp;
        i++;
    }
}

void newNode(Tree* tree, Evaluation* e)
{
    Node* left = e->sort_char[e->i];
    Node* right = (e->nb_distinct_char > 1) ? e->sort_char[e->i + 1] : newLeaf(0, 0);

    if (!left->is_node)
    {
        tree->leafs[left->value] = left;
    }

    if (!right->is_node)
    {
        tree->leafs[right->value] = right;
    }

    Node* node = malloc(sizeof(Node));
    tree->root = node;
    node->parent = NULL;
    node->is_node = true;
    node->occurence = left->occurence + right->occurence;
    node->left = left;
    node->right = right;
    left->parent = node;
    left->bit = false;
    right->parent = node;
    right->bit = true;

    classTree(e, node);
}

void serializeTree(Tree* tree, FileContent* f)
{
    Node* node;
    Node** temp_queue = malloc(2 * sizeof(Node));
    unsigned int i;
    unsigned int j = 0;
    unsigned int nb_node = 1;
    unsigned int mask = 1;

    while (nb_node != 0 && f->file_size < f->max_size)
    {
        node = tree->node_search[tree->next_node];
        tree->next_node++;

        if (node->is_node)
        {
            addBit(f, false);
            temp_queue[j] = node->left;
            j++;
            temp_queue[j] = node->right;
            j++;
        }
        else
        {
            mask = 128;
            addBit(f, true);

            for (i = 0; i < 8; i++)
            {
                addBit(f, ((node->value & mask) == mask));
                mask /= 2;
            }
        }

        if (tree->next_node >= nb_node)
        {
            nb_node = j;
            j = 0;
            tree->next_node = 0;
            free(tree->node_search);
            tree->node_search = temp_queue;
            temp_queue = malloc(nb_node * 2 * sizeof(Node));
        }
    }

    free(tree->node_search);
    free(temp_queue);
}

Tree* unserializeTree(FileContent* f, size_t* seek)
{
    Tree* tree = newTree();
    Node** temp_queue = malloc(2 * sizeof(Node));
    Node* n;
    unsigned int nb_node = 1;
    unsigned int i = 0;
    unsigned int j = 0;
    unsigned char c;
    bool t;

    while (nb_node != 0 && compressionEnd(f, *seek, 0))
    {
        c = 0;

        if (getBit(f, seek))
        {
            for (unsigned k = 128; k > 1; k /= 2)
            {
                t = getBit(f, seek);
                c += t ? k : 0;
            }

            t = getBit(f, seek);
            c += t ? 1 : 0;
            n = newLeaf(c, 0);
        }
        else
        {
            n = malloc(sizeof(Node));
            n->is_node = true;
            temp_queue[j] = n;
            j++;
            temp_queue[j] = n;
            j++;
        }

        if ((i % 2) == 0 && tree->root != NULL)
        {
            n->bit = false;
            tree->node_search[i]->left = n;
        }
        else if (tree->root != NULL)
        {
            n->bit = true;
            tree->node_search[i]->right = n;
        }
        else if (tree->root == NULL && n->is_node)
        {
            tree->root = n;
        }
        else
        {
            return NULL;
        }

        n->parent = tree->node_search[i];
        i++;

        if (i >= nb_node)
        {
            nb_node = j;
            i = 0;
            j = 0;
            free(tree->node_search);
            tree->node_search = temp_queue;
            temp_queue = malloc(nb_node * sizeof(Node));
        }
    }

    free(tree->node_search);
    free(temp_queue);

    return compressionEnd(f, i, 0) ? tree : NULL;
}

int main_c(FileContent* f, FileContent* c)
{
    printf("Evaluation du fichier...\n");
    Evaluation *e = evaluate(f);

    if (e == NULL)
    {
        printf("Erreur N°4 : Une erreur inattendu s'est produite !\n");

        return 4;
    }

    printf("Création du dictionnaire...\n");
    Tree* tree = newTree();
    newNode(tree, e);

    while (e->i < (e->nb_distinct_char - 1))
    {
        newNode(tree, e);
    }

    tree->node_search[0] = tree->root;
    printf("Ecriture du dictionnaire...\n");
    serializeTree(tree, c);
    size_t i = 0;
    unsigned int j;
    Node* n;
    bool* temp = malloc(256);
    printf("Compression...\n");

    while (i < f->file_size && c->file_size < c->max_size)
    {
        nextBlock(f, false, i);
        n = tree->leafs[f->content[f->temp_seek]];
        f->temp_seek++;
        j = 0;

        while (n->parent != NULL)
        {
            temp[j] = n->bit;
            n = n->parent;
            j++;
        }

        for (unsigned int k = (j - 1); k > 0; k--)
        {
            addBit(c, temp[k]);
        }

        addBit(c, temp[0]);
        i++;
    }

    if (c->file_size >= c->max_size)
    {
        remove(c->location);
        printf("Erreur N°5 : Le fichier n'est pas compressible !\n");

        return 5;
    }

    if (c->byte_seek != 128)
    {
        c->content[c->temp_seek] = c->c;
        c->temp_seek++;
        c->file_size++;
        unsigned char init[1] = {c->byte_seek};
        fseek(c->fp, 0, SEEK_SET);
        fwrite(init, sizeof(unsigned char), 1, c->fp);
    }

    fseek(c->fp, 0, SEEK_END);
    fwrite(c->content, sizeof(unsigned char), c->temp_seek, c->fp);
    printf("Compression terminé !\n");
    printf("Taille fichier source : %.2f KO | Taille fichier compressé : %.2f KO\n", (float)f->file_size / 1024.00, (float)c->file_size / 1024.00);
    printf("Taux de compression : %lf\n", 1 - ((double)c->file_size / (double)f->file_size));

    return 0;
}

int main_d(FileContent* f, FileContent* d)
{
    size_t i = 1;
    unsigned char ignore = f->content[0];
    f->temp_seek++;
    printf("Lecture du dictionnaire...\n");
    Tree* tree = unserializeTree(f, &i);

    if (tree == NULL)
    {
        printf("Ce fichier n'est pas un fichier décompressible !\n");

        return 6;
    }

    Node* n;
    printf("Décompression...\n");

    while (compressionEnd(f, i, ignore))
    {
        n = tree->root;

        while (n->is_node && compressionEnd(f, i, 0))
        {
            n = (!getBit(f, &i)) ? n->left : n->right;
        }

        if (!n->is_node)
        {
            d->content[d->temp_seek] = n->value;
            d->max_size++;
            d->temp_seek++;
            nextBlock(d, true, 0);
        }
    }

    fwrite(d->content, sizeof(unsigned char), d->temp_seek, d->fp);
    printf("Fichier décompressé !\n");

    return 0;
}

int main(int const argc, char const* argv[])
{
    if (argc != 4 && strlen(argv[2]) == 2)
    {
        printf("Erreur N°1 : Les arguments envoyé au programme sont erroné !\n");
        printf("Vous devez entrer la commande sous la forme suivante :\n");
        printf(".../huffman fichier_entrée -c/-d fichier_sortie\n");

        return 1;
    }

    FileContent *f = getFileContent(argv[1], false, 0);

    if (f->file_size == 0)
    {
        printf("Erreur N°2 : Le fichier demandé est vide ou n'existe pas !\n");

        return 2;
    }

    if (fileExist(argv[3]))
    {
    	char file_exist;
    	printf("Le fichier %s existe déjà, voulez-vous vraiment l'écraser ? (o/n)", argv[3]);
    	scanf("%c", &file_exist);

    	if (file_exist == 'n')
    	{
            printf("La compression a été annulé\n");

            return 0;
    	}
    }

    FileContent *c = getFileContent(argv[3], true, (argv[2][1] == 'c') ? f->file_size : 0);

    if (argv[2][1] == 'c')
    {
        unsigned char init[1] = {0};
        fwrite(init, 1, 1, c->fp);

    	return main_c(f, c);
    }

    if (argv[2][1] == 'd')
    {
    	return main_d(f, c);
    }
 
    printf("Erreur N°3 : Vous devez entrez des paramètres valides : fichiersource -c destination > compresser | fichiercompresse -d destination > décompresser\n");

    return 3;
}
