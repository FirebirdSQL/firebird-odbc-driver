/*
 *	PROGRAM:		Virtual Data Manager
 *	MODULE:			LinkedList.c
 *	DESCRIPTION:	Generic Linked List
 *
 * copyright (c) 1997 by James A. Starkey for IBPhoenix.
 */

#include <stdio.h>
//#include "Engine.h"
#include "LinkedList.h"

#ifdef _DEBUG
static char THIS_FILE[]=__FILE__;
#endif

/***
    FOR_OBJECTS(type,child,list) 
		{
		for (LinkedList *pos=(list)->getHead();
			  (list)->more (pos);)
		    {
			type child = (type) (list)->getNext (&pos);
***/

LinkedList::LinkedList ()
{
/**************************************
 *
 *		L i n k e d L i s t
 *
 **************************************
 *
 * Functional description
 *		Constructor for both nodes and list head.
 *
 **************************************/

//addressCheck (this);

next = prior = NULL;
}

LinkedList::~LinkedList ()
{
/**************************************
 *
 *		~ L i n k e d L i s t
 *
 **************************************
 *
 * Functional description
 *		Constructor for both nodes and list head.
 *
 **************************************/
LinkedNode *node;

//addressCheck (this);

while (node = next)
    {
	next = node->next;
	delete node;
	}

}

LinkedNode::LinkedNode (void *obj)
{
/**************************************
 *
 *		L i n k e d N o d e
 *
 **************************************
 *
 * Functional description
 *		Constructor for both nodes and list head.
 *
 **************************************/

object = obj;
}

LinkedNode::~LinkedNode ()
{
/**************************************
 *
 *		~ L i n k e d N o d e
 *
 **************************************
 *
 * Functional description
 *		Constructor for both nodes and list head.
 *
 **************************************/

next = prior = NULL;
}

void LinkedList::append (void *object)
{
/**************************************
 *
 *		a p p e n d
 *
 **************************************
 *
 * Functional description
 *		Append object/node to list.
 *
 **************************************/

LinkedNode *node = new LinkedNode (object);
addressCheck (node);

if (prior)
    {
	prior->next = node;
	node->prior = prior;
	}
else
	next = node;

prior = node;
}

int LinkedList::appendUnique (void *object)
{
/**************************************
 *
 *		a p p e n d U n i q u e
 *
 **************************************
 *
 * Functional description
 *		Append object/node to list.
 *
 **************************************/
int			n;
LinkedNode	*node;

for (node = next, n = 0; node; node = node->next, n++)
    if (node->object == object)
		return n;

append (object);

return n;
}

void LinkedList::clear ()
{
/**************************************
 *
 *		c l e a r
 *
 **************************************
 *
 * Functional description
 *		Delete all elements of list.
 *
 **************************************/
LinkedNode	*node;

while (node = next)
    {
	next = node->next;
	delete node;
	}

prior = NULL;
}

int LinkedList::count ()
{
/**************************************
 *
 *		c o u n t
 *
 **************************************
 *
 * Functional description
 *		Count nodes.
 *
 **************************************/
int	n = 0;

for (LinkedList *node = next; node; node = node->next)
    ++n;

return n;
}

bool LinkedList::deleteItem (void *object)
{
/**************************************
 *
 *		d e l e t e I t e m
 *
 **************************************
 *
 * Functional description
 *		Delete node from linked list.  Return true is the item
 *		was on the list, otherwise return false.
 *
 **************************************/

for (LinkedNode *node = next; node; node = node->next)
    if (node->object == object)
		{
		if (node->prior)
			node->prior->next = node->next;
		else
			next = node->next;
		if (node->next)
			node->next->prior = node->prior;
		else
			prior = node->prior;
		delete node;
		return true;
		}

return false;
}

void *LinkedList::getElement (int position)
{
/**************************************
 *
 *		g e t E l e m e n t
 *
 **************************************
 *
 * Functional description
 *		Get element in list by position.
 *
 **************************************/
int			n;
LinkedNode	*node;

for (node = next, n = 0; node; node = node->next, ++n)
	if (n == position)
		return node->object;

return NULL;
}

#ifdef UNDEF
LinkedList *LinkedList::getHead ()
{
/**************************************
 *
 *		g e t H e a d
 *
 **************************************
 *
 * Functional description
 *		Get first list node; used for iterating.
 *
 **************************************/

return next;
}

void *LinkedList::getNext (LinkedList **node)
{
/**************************************
 *
 *		g e t N e x t
 *
 **************************************
 *
 * Functional description
 *		Return object of current node; advance pointer.
 *
 **************************************/

void *object = ((LinkedNode*)(*node))->object;
*node = (*node)->next;

return object;
}
#endif

void *LinkedList::getPrior (LinkedList **node)
{
/**************************************
 *
 *		g e t P r i o r
 *
 **************************************
 *
 * Functional description
 *		Return object of current node; advance pointer.
 *
 **************************************/
//void	*object;

LinkedNode *n = (*node)->prior;
*node = n;
//*node = (*node)->prior;
//object = (*node)->object;

return n->object;
}

LinkedList *LinkedList::getTail ()
{
/**************************************
 *
 *		g e t T a i l
 *
 **************************************
 *
 * Functional description
 *		Get last list node; used for iterating.
 *
 **************************************/

return this;
}

bool LinkedList::isEmpty ()
{
/**************************************
 *
 *		i s E m p t y
 *
 **************************************
 *
 * Functional description
 *		Return whether iteration has more members to go.
 *
 **************************************/

return next == NULL;
}

bool LinkedList::isMember (void *object)
{
/**************************************
 *
 *		i s M e m b e r
 *
 **************************************
 *
 * Functional description
 *		Is object in list?
 *
 **************************************/

for (LinkedNode *node = next; node; node = node->next)
    if (node->object == object)
		return 1;

return 0;
}

#ifdef UNDEF
bool LinkedList::more (LinkedList *node)
{
/**************************************
 *
 *		m o r e
 *
 **************************************
 *
 * Functional description
 *		Return whether iteration has more members to go.
 *
 **************************************/

return (node != NULL);
}
#endif

bool LinkedList::moreBackwards (LinkedList *node)
{
/**************************************
 *
 *		m o r e B a c k w a r d s
 *
 **************************************
 *
 * Functional description
 *		Return whether iteration has more members to go.
 *
 **************************************/

return (node->prior != NULL);
}

bool LinkedList::insertBefore(void * insertItem, void * item)
{
/**************************************
 *
 *		i n s e r t B e f o r e
 *
 **************************************
 *
 * Functional description
 *		Add item in middle of list.
 *
 **************************************/

for (LinkedNode *node = next; node; node = node->next)
    if (node->object == item)
		{
		LinkedNode *insert = new LinkedNode (insertItem);
		if (insert->prior = node->prior)
			insert->prior->next = insert;
		else
			next = insert;
		node->prior = insert;
		insert->next = node;
		return true;
		}

return false;
}

void LinkedList::addressCheck(void * address)
{

	if ((long) address == 0x00CDE290 ||
		(long) address == 0x00cdde70)
		printf ("hit %x\n", address);
}
