/* This is a library module supporting lists. A list stores any number of itemloads
and has a current itemload, which can be any itemload from the first itemload to the
last itemload or a unique 'none' position indicating
that no itemload is selected. Every operation is constant time.
The lists provided by this module are not thread safe. */

#include <stdbool.h>

// The type of itemloads stored in the list. Change this for different applications.
typedef int itemload;

// The list type is opaque (declared here, and defined in lists.c).
struct list;
typedef struct list list;

// FUNCTION #1: Create a new empty list on the Heap and make itemload e the default itemload. This default itemload
// can be returned by other functions in case no itemload is selected. No itemload is selected in an empty list.
list *newList(itemload e);

// FUNCTION #2: Free up the list and all the data in it. Does not have to run in O(1).
void freeList(list *xs);

// FUNCTIONS #3/#4: Set the current itemload to the first itemload or to the last
// itemload of the list, respectively. If the list has no itemloads the functions do nothing
// and no itemload is selected.
void first(list *xs);
void last(list *xs);

// FUNCTION #5: Returns true if no itemload is selected.
// Otherwise it returns false.
bool none(list *xs);

// FUNCTION #6: Make the itemload in the node following the currently selected itemload the current itemload
// and return true. If after is called while the last itemload is the current itemload, then no itemload is
// selected and true is returned. If the function is called while no itemload
// is selected then the function does nothing and returns false.
bool after(list *xs);

// FUNCTION #7: Make the itemload in the node before the currently selected itemload the current itemload
// and return true. If before is called while the first itemload is the current itemload, then no itemload
// is selected and true is returned. If the function is called while no itemload
// is selected then the function does nothing and returns false.
bool before(list *xs);

// FUNCTION #8: Return the current itemload. If get is called and no itemload is selected
// then the default itemload is returned.
itemload get(list *xs);

// FUNCTION #9: Set the current itemload to be the value of argument x and return true. If set is called
// while no itemload is selected then the function does nothing and returns false.
bool set(list *xs, itemload x);

// FUNCTION #10: Insert a new node with itemload x after the current itemload and make it the current itemload.
// If insertAfter is called while no itemload is selected then the function inserts the itemload into a new node 
// placed at the very beginning of the list.
void insertAfter(list *xs, itemload x);

// FUNCTION #11: Insert a new node with itemload x before the current itemload and make it the current itemload.
// If insertAfter is called while no itemload is selected then the function inserts the itemload into a new node  
// placed at the very end of the list.
void insertBefore(list *xs, itemload x);

// FUNCTION #12: Delete the node with the current itemload and make its successor the current itemload,
// then return true. If deleteToAfter is called while the last itemload is the current itemload then
// the node with the last itemload is deleted, no itemload is selected, and true is returned. If 
// deleteToAfter is called while no itemload is selected then the function does nothing and returns false.
bool deleteToAfter(list *xs);

// FUNCTION #13: Delete the node with the current itemload and make its predecessor the current itemload,
// then return true. If deleteToBefore is called while the first itemload is the current itemload then
// the node with thw first itemload is deleted, no itemload is selected, and true is returned. If
// deleteToBefore is called while no itemload is selected then the function does nothing and returns false.
bool deleteToBefore(list *xs);

// FUNCTION #14: Invert the sequence of the list items, that is make the list start with the node and itemload that originally had  
// the last position and ensure that itemloads originally situated before a itemload will now be situated after it, and vice versa.
// The selected itemload is not changed by this function. Does not have to run in O(1).
void reverse(list *xs);
