/*
 *	PROGRAM:		Virtual Data Manager
 *	MODULE:			LinkedList.h
 *	DESCRIPTION:	Generic Linked List
 *
 * copyright (c) 1997 by James A. Starkey for IBPhoenix.
 */

#ifndef __LINKEDLIST_H
#define __LINKEDLIST_H

#define FOR_OBJECTS(type,child,list) {for (LinkedList *pos=(list)->getHead();(list)->more (pos);){ type child = (type) (list)->getNext (&pos);
#define FOR_OBJECTS_BACKWARD(type,child,list) {for (LinkedList *pos=(list)->getTail();(list)->moreBackwards (pos);){ type child = (type) (list)->getPrior (&pos);
#define END_FOR						 }}

#ifndef NULL
#define NULL	0
#endif

class LinkedNode;

class LinkedList
    {
	public:
		void addressCheck (void *address);
		bool insertBefore (void *insertItem, void *item);

	LinkedList();
	~LinkedList();

	void		append (void *object);
	int			appendUnique (void *object);
	void		clear();
	int			count ();
	bool		deleteItem (void* object);

	LinkedList	*getHead ();
	bool		more (LinkedList *node);
	void		*getNext(LinkedList **node);

	LinkedList	*getTail ();
	void		*getElement (int position);
	void		*getPrior(LinkedList**);
	bool		isEmpty();
	bool		isMember (void *object);
	
	bool		moreBackwards (LinkedList*);

	protected:
	
	LinkedNode		*next, *prior;
	//void			*object;
	};

class LinkedNode : public LinkedList {
	public:

	LinkedNode (void *object);
    ~LinkedNode();

	void			*object;
	};

inline LinkedList *LinkedList::getHead ()
	{
	return (LinkedList*) next;
	}

inline bool	LinkedList::more (LinkedList *node)
	{
	return (node != NULL);
	}

inline void	*LinkedList::getNext(LinkedList **node)
	{
	void *object = ((LinkedNode*)(*node))->object;
	*node = (*node)->next;

	return object;
	}


#endif

