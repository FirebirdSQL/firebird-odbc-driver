/*
 *  
 *     The contents of this file are subject to the Initial 
 *     Developer's Public License Version 1.0 (the "License"); 
 *     you may not use this file except in compliance with the 
 *     License. You may obtain a copy of the License at 
 *     http://www.ibphoenix.com/idpl.html. 
 *
 *     Software distributed under the License is distributed on 
 *     an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either 
 *     express or implied.  See the License for the specific 
 *     language governing rights and limitations under the License.
 *
 *
 *  The Original Code was created by Vladimir Tsvigun for IBPhoenix.
 *
 *  Copyright (c) 2003 Vladimir Tsvigun
 *  All Rights Reserved.
 */

// Mlist.h
//
//////////////////////////////////////////////////////////////////////

#if !defined(_MLIST_H_)
#define _MLIST_H_

#include <malloc.h>

template <class T>
class MList
{
private:
	T	 	 *	m_Root, // Head list
			 *	pActiv; // active list node
	long		m_nCount;	// count node
	char		bOk;
	long		m_nKolStartKesh; 
	long		m_nKolKesh;		 
	long		m_nPozActiv;
private:
	void OnInitList()
	{
		bOk=false;
		m_nCount = 0;
		m_nKolKesh=m_nKolStartKesh;m_nPozActiv=0;pActiv=NULL;
		m_Root=(T*)calloc(1,m_nKolKesh * sizeof(T));
		if(m_Root)
			bOk=true;
	}
protected:
	void SetActivItem(int nPoz)
	{
		m_nPozActiv=nPoz;
		pActiv=&m_Root[nPoz]; // active list node
	}
public:
	MList(T &t)
	{
		m_nKolStartKesh = 20;
		OnInitList();
		Attach(t);
	}
	MList()
	{
		m_nKolStartKesh = 50;
		OnInitList();
	}
	MList(long nCountKesh)
	{
		m_nKolStartKesh = nCountKesh;
		OnInitList();
	}
	void OnRemoveAll()
	{
		OnDelete();
		OnInitList();
	}
	void OnDelete();
	virtual ~MList()
	{
		OnDelete();
	}
	char OnReinit()
	{
		if(bOk==false)return false;
		char bRez=false;
		T * tmp;
		tmp=(T*)realloc(m_Root,(m_nKolKesh + 50) * sizeof(T));
		if(tmp)
		{
			m_nKolKesh+=50;
			m_Root=tmp;
			bRez=true;
		}
		return bRez;
	}
	T & operator [](int nPoz){ return  m_Root[nPoz];}
	T & GetNode(int nPoz){ return  m_Root[nPoz];}
	void operator =(MList &Lst)
	{
		if(Lst.GetCount()==0)
			return;
		OnDelete();
		m_Root=(T*)calloc(1,Lst.m_nKolKesh * sizeof(T));
		if(m_Root)
		{
			bOk=true;
			m_nCount=Lst.m_nCount;
			m_nKolKesh=Lst.m_nKolKesh;
			m_nKolStartKesh=Lst.m_nKolStartKesh;
			memcpy(m_Root,Lst.m_Root,sizeof(T)*m_nCount);
		}
		else
			bOk=false;
	}
	char OnAttach(MList &Lst)
	{
		if(Lst.GetCount()==0)
			return false;

		OnDelete();
		m_Root=Lst.m_Root;
		m_nCount=Lst.m_nCount;
		m_nKolKesh=Lst.m_nKolKesh;
		m_nKolStartKesh=Lst.m_nKolStartKesh;
		Lst.m_Root=NULL;
		Lst.OnDelete();
		return true;
	}
	void Serialize(char bSave,void * buf,long &nLen);
	long GetSizeSerialize();
	char GetSuccess(){ return bOk; }
	T * GetRoot(){ return m_Root; }
	int GetCount(){ return m_nCount; }
	int GetSize() { return m_nCount * sizeof(T); }
	T * GetActiv(){ return pActiv; }
	int GetPozActiv(){ return m_nPozActiv; }

	T *GetHeadPosition(int nStartPoz=0)
	{
		if(m_nCount==0)return NULL;
		if(nStartPoz<m_nCount)
		{
			pActiv=&m_Root[nStartPoz];
			m_nPozActiv=nStartPoz;
		}
		else
		{
			pActiv=m_Root;
			m_nPozActiv=0;
		}
		return pActiv;
	}
	T *GetNext()
	{
		if(m_nPozActiv+1>=m_nCount)return NULL;
		pActiv=&m_Root[++m_nPozActiv];
		return pActiv;
	}
	T *GetTailPosition()
	{
		if(m_nCount==0)return NULL;
		pActiv=&m_Root[m_nCount-1];
		m_nPozActiv=m_nCount-1;
		return pActiv;
	}
	T *GetPrev()
	{
		if(m_nPozActiv<1)return NULL;
		pActiv=&m_Root[--m_nPozActiv];
		return pActiv;
	}
	long SearchAndInsert(const void * klych,long (*Cmp)(const void *,const void *));
	long Search(const void * klych,long (*Cmp)(const void *,const void *));
	void DeleteAt(long poz);
	void Add(const T &t);
	void Attach(T &t);
	void operator <<(T &t);
	void operator +=(const T &t) { Add(t); }
};

template <class T>
void MList<T>::OnDelete()
{
	if(m_Root)
	{
		T * p = m_Root;
		for(long i=0;i<m_nCount;i++,p++)p->Remove();
		free(m_Root);m_Root = NULL;
	}
	m_nCount = 0; m_nKolKesh=m_nKolStartKesh;
}
template <class T>
void MList<T>::operator <<(T &t)
{
	Attach(t);
}
template <class T>
void MList<T>::Attach(T &t)
{
	if(m_nCount==m_nKolKesh)OnReinit();
	m_Root[m_nCount++] << t;
}

template <class T>
void MList<T>::Add(const T &t)
{
	if(m_nCount==m_nKolKesh)OnReinit();
	m_Root[m_nCount++]=t;
}

template <class T>
long MList<T>::SearchAndInsert(const void * klych,long (*Cmp)(const void *,const void *))
{
	register long cmp=1,i,l=0,u=m_nCount-1;
	long size = sizeof(T);
	T * buf=m_Root;
	while(u>=l)
	{
		i=(l+u)/2;
		if(cmp=Cmp(klych,(const void*)&buf[i]),cmp<0)u=i-1;
		else if (cmp>0)l=i+1;
		else break;
	}
	if(cmp)
	{
		if(cmp>0)i=l;
		else i=u+1;
		memmove(&buf[i+1],&buf[i],(m_nCount-i)*size);
		memset(&buf[i],0,size);
		m_nCount++;i++;
		if(m_nCount==m_nKolKesh)OnReinit();
		i=-i;
	}
	return i;
}

template <class T>
long MList<T>::Search(const void * klych,long (*Cmp)(const void *,const void *))
{
	register long cmp=1,i,l=0,u=m_nCount-1;
	T * buf=m_Root;
	while(u>=l)
	{
		i=(l+u)/2;
		if(cmp=Cmp(klych,(const void*)&buf[i]),cmp<0)u=i-1;
		else if (cmp>0)l=i+1;
		else break;
	}
	if(cmp)i=-1;
	return i;
}
template <class T>
void MList<T>::DeleteAt(long poz)
{
	if(m_nCount==0)return;
	if(poz == m_nCount-1)
	{
		memset(&m_Root[poz],0,sizeof(T));
		m_nCount--;
		return;
	}
	m_nCount--;
	memmove(&m_Root[poz],&m_Root[poz+1],(m_nCount-poz)*sizeof(T));
	memset(&m_Root[m_nCount],0,sizeof(T));
}
template <class T>
long MList<T>::GetSizeSerialize()
{
	long siz=sizeof(long)*2;
	if(m_Root!=NULL)
		siz+=m_Root->GetSizeSerialize()*m_nCount;
	return siz;
}
template <class T>
void MList<T>::Serialize(char bSave,void * p,long &nLen)
{
	long sizeData;
	char * buf=(char *)p;
	char * pPointBufStart=buf;
	long siz;
	if(bSave)
	{
		siz=sizeof(m_nCount);memcpy(buf,&m_nCount,siz);buf+=siz;
		if(m_Root!=NULL)sizeData=m_Root->GetSizeSerialize()*m_nCount;
		else sizeData=0;
		siz=sizeof(sizeData);memcpy(buf,&sizeData,siz);buf+=siz;
	}
	else
	{
		siz=sizeof(m_nCount);memcpy(&m_nCount,buf,siz);buf+=siz;
		siz=sizeof(sizeData);memcpy(&sizeData,buf,siz);buf+=siz;
		bOk=false;
		m_nKolKesh=m_nCount+10;
		m_nPozActiv=0;pActiv=NULL;
		if(m_Root)farfree(m_Root);
		m_Root=(T*)calloc(1,m_nKolKesh * sizeof(T));
		if(m_Root)bOk=true;
	}
	nLen+=(long)(buf-pPointBufStart);
	if(m_Root)
	{
		long len=0;
		T * p = m_Root;
		for(long i=0;i<m_nCount;i++,p++)
			p->Serialize(bSave,(void*)buf,len);
		nLen+=len;
	}
}

#endif // !defined(_MLIST_H_)
