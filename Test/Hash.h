/*
 *	PROGRAM:		Schema Converter
 *	MODULE:			Hash.h
 *	DESCRIPTION:	Hash table stuff
 *
 * copyright (c) 1997 by James A. Starkey for IBPhoenix.
 */

#ifndef __HASH_H
#define __HASH_H

#ifndef TRUE
#define TRUE		1
#define FALSE		0
#endif

class Hash
    {
	public:
		bool isFirst (const char* string);
		Hash (int size);
		~Hash ();

		void		deleteAll();
		void		insert (const char *string, void *object);
		void		*lookup (const char *string);

		void		*initWalk ();
		int			next (void *marker);
		const char	*getString (void *marker);
		void		*getObject (void *marker);
		void		finiWalk (void *marker);

	protected:
		int		hash (const char *string);

		struct HashEntry
			{
			~HashEntry();
			char		*string;
			void		*object;
			HashEntry	*next;
			};

		struct HashWalk
			{
			int		slot;
			HashEntry	*node;
			};

		HashEntry	**table;
		int			tableSize;
	};


#endif
