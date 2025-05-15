/* File: list.c
 * Author: Richard Hsin

*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct List_node_s
{
    int length;
    char *word;
    struct List_node_s *next;
} List_node;

// Function to create a new node
List_node *createNode(const char *word)
{
    List_node *newNode = (List_node *)malloc(sizeof(List_node));
    if (!newNode)
    {
        perror("Failed to allocate memory");
        exit(EXIT_FAILURE);
    }
    newNode->word = strdup(word);
    newNode->length = strlen(word);
    newNode->next = NULL;
    return newNode;
}

// Function to insert a word into the list
void add_word(List_node **head, const char *word)
{
    List_node *newNode = createNode(word);

    if (*head == NULL || newNode->length < (*head)->length)
    {
        newNode->next = *head;
        *head = newNode;
        return;
    }

    List_node *current = *head;
    while (current->next != NULL && current->next->length <= newNode->length)
    {
        current = current->next;
    }

    newNode->next = current->next;
    current->next = newNode;
}

// Function to print the list
void printList(List_node *head)
{
    List_node *current = head;
    while (current != NULL)
    {
        printf("%s\n", current->word);
        current = current->next;
    }
}

// Function to free the list
void freeList(List_node *head)
{
    List_node *temp;
    while (head != NULL)
    {
        temp = head;
        head = head->next;
        free(temp->word);
        free(temp);
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Usage: %s <words...>\n", argv[0]);
        return EXIT_FAILURE;
    }

    List_node *head = NULL;

    for (int i = 1; i < argc; i++)
    {
        add_word(&head, argv[i]);
    }

    printList(head);
    freeList(head);

    return EXIT_SUCCESS;
}
