#ifndef __LIST__
#define __LIST__

//KLASA LIST<T,P,types>
//Argumenty wzorca:
//T: typ elementu skladowanego w liscie
//P: wyliczenie okreslajace porzadek elementow
//types: liczba poszczegolnych stalych w wyliczneniu
//UWAGA: element typu T musi miec pola:
//T *prev;
//T *next;
//P prty;

#if HI_DEBUG
void List_statistics(void);
void update_list_statistics(int usage,char* user_name);
#endif

template<class T,class P,int types> class List  : public Heap_object
{
  private:
#if HI_DEBUG
   char *user;
#endif
   int removed;
   T *head,*curr;
   T *first[types];
   int objects_numb;
  public:
   List(char *_user);
   ~List(void);
   int size(void);
   void add(T* which,P prty);
   void add_end(T* which,P prty);
   void remove(T* which);
   T* reset(void);
   T* get_next(void);
};

template<class T,class P,int types> List<T,P,types>::List(char *_user)
{
#if HI_DEBUG
  user=_user;
#else
  (void)_user;
#endif
  objects_numb=0;
  removed=0;
  head=NULL;
  curr=NULL;
  for (int i=0;i<types;i++) first[i]=NULL;
}

template<class T,class P,int types> List<T,P,types>::~List(void)
{
  DBG_CHECK(objects_numb==0);
}

template<class T,class P,int types> int List<T,P,types>::size(void)
{
  return objects_numb;
}

template<class T,class P,int types> void List<T,P,types>::add(T* which,P prty)
{
  objects_numb++;
  DBG_CHECK((int)prty>=0&&(int)prty<types);
#if HI_DEBUG
  update_list_statistics(objects_numb,user);
#endif
  which->prev=which->next=NULL;
  which->prty=prty;
  if (head==NULL)
  {
    head=which;
    first[prty]=which;
  }
  else
  {
    T *temp1=NULL,*temp2=NULL;
    if (first[prty]==NULL)
    {
      first[prty]=which;
      temp1=head,temp2=NULL;
      while (temp1!=NULL&&temp1->prty<prty)
      {
        temp2=temp1;
        temp1=temp1->next;
      }
    }
    else
    {
      temp1=first[prty];
      first[prty]=which;
      temp2=temp1->prev;
    }
    which->prev=temp2;
    which->next=temp1;
    if (temp1!=NULL) temp1->prev=which;
    if (temp2!=NULL) temp2->next=which;
    if (temp1==head) head=which;
  }
}

template<class T,class P,int types> void List<T,P,types>::add_end(T* which,P prty)
{
  objects_numb++;
  DBG_CHECK((int)prty>=0&&(int)prty<types);
#if HI_DEBUG
  update_list_statistics(objects_numb,user);
#endif
  which->prev=which->next=NULL;
  which->prty=prty;
  if (head==NULL)
  {
    head=which;
    first[prty]=which;
  }
  else
  {
    T *temp1=NULL,*temp2=NULL;
    if (first[prty]==NULL)
    {
      first[prty]=which;
      temp1=head,temp2=NULL;
      while (temp1!=NULL&&temp1->prty<prty)
      {
        temp2=temp1;
        temp1=temp1->next;
      }
    }
    else
    {
      temp1=first[prty],temp2=NULL;
      while (temp1!=NULL&&temp1->prty<=prty)
      {
        temp2=temp1;
        temp1=temp1->next;
      }
    }
    which->prev=temp2;
    which->next=temp1;
    if (temp1!=NULL) temp1->prev=which;
    if (temp2!=NULL) temp2->next=which;
    if (temp1==head) head=which;
  }
}

template<class T,class P,int types> void List<T,P,types>::remove(T* which)
{
  for (T* temp=head;temp!=NULL&&temp!=which;temp=temp->next);
  DBG_CHECK(temp!=NULL);
  DBG_CHECK((int)which->prty>=0&&(int)which->prty<types);
  objects_numb--;
  if (which->next!=NULL) which->next->prev=which->prev;
  if (which->prev!=NULL) which->prev->next=which->next;
  if (which==head) head=head->next;
  removed=(which==curr);
  if (removed)
  {
    curr=curr->next;
  }
  if (first[which->prty]==which)
  {
    T *temp1=which->next;
    if (temp1!=NULL&&temp1->prty==which->prty)
    {
      first[which->prty]=temp1;
    }
    else
    {
      first[which->prty]=NULL;
    }
  }
}

template<class T,class P,int types> T* List<T,P,types>::reset(void)
{
  removed=0;
  curr=head;
  return curr;
}

template<class T,class P,int types> T* List<T,P,types>::get_next(void)
{
  if (curr!=NULL)
  {
    if (removed)
    {
      removed=0;
    }
    else
    {
      curr=curr->next;
    }
  }
  return curr;
}

#endif







