#include "construction.h"

#include <stdio.h>

void test(const int i)
{
	printf("test %d\n", i);
}

/**
 * retourne s'il n'y a plus de point à ajouter*/
int insertPoint(pqueue* pq, tstack* pile, settings* s)
{
  if(pq->items[1]->candidats == NULL)
    return (0);

  switch (s->stop_mode)
  {
  case STOPMODE_FACE_COUNT:
    if (pq->size >= s->max_face_count)
      return (0);
    break;

  case STOPMODE_GOF:
	if (pq->items[1]->distance_max < (1-s->gof)*s->altitude_max)
		return (0);

    break;
  }

  triangle* t = pqueue_dequeue(pq);
  vertex* p = t->candidats;
  creerTroisTriangles(t, pq, pile);
  corrigeTriangles(pile, p, pq);

  return (1);
}

void initCarre(vertex* premier, pqueue* pq)
{
	assert(premier != NULL);
	const int naturel = VLINK_NATURAL,	suiv = VLINK_FORWARD;
	vertex* v = premier;
	vertex* v2 = v->link[naturel][suiv], *v3 = v2->link[naturel][suiv];
	
	triangle** tgls = (triangle**) malloc(sizeof(triangle*)*2);
	tgls[0] = triangle_create2(v, v3, v2);		//CCW
	tgls[1] = triangle_create2(v2, v3, v3->link[naturel][suiv]);	//CCW
	tgls[0]->v[0] = tgls[1];
	tgls[1]->v[2] = tgls[0];
	
	v = v3->link[naturel][suiv]->link[naturel][suiv];	//v pointe vers le premier vertex qui n'ai pas un sommet du carré
	recopieType(v, naturel, VLINK_CANDIDAT);
	repartageCandidats(tgls, 2, v);
	
	pqueue_enqueue(pq, tgls[0]);
	pqueue_enqueue(pq, tgls[1]);
	free(tgls);
}

/**met le vertex *v dans les candidats du triangle s'il est à l'intérieur*/
int repartage(triangle* t, vertex* v)
{
	int sur = dansTriangle2d(t, v);
	if(sur == 2)	//sur une arête
		printf("point placé pile sur un bord d'un triangle.\n");//exit(-1);
	//if(sur > 0) //il est dans le triangle, mais il peut aussi être sur la limite entre les deux triangles
	else if(sur == 1)
	{
		ajouteCandidat(t, v);
		return 1;
	}
	return 0;
}

void repartageCandidats(triangle** tgls, const int nbTriangles, vertex* candid)
{
	const int candidat = VLINK_CANDIDAT,	suiv = VLINK_FORWARD;
	vertex* v = candid, *suivant;
	while(v != NULL)
	{
		//bien mettre le suivant de chaque candidat à NULL;
		suivant = v->link[candidat][suiv];
		v->link[candidat][suiv] = NULL;
		
		int i = 0;
		//mettre le candidat dans l'un des triangles.
		while(i < nbTriangles && !repartage(tgls[i], v))
			i++;
		
		v = suivant;
	}
}

/**créer trois triangles en découpant un triangle par son premier candidat*/
void creerTroisTriangles(triangle* t, pqueue* pq, tstack* pile)
{	
	triangle** tgls = (triangle**) malloc(sizeof(triangle*)*3);
	vertex* v = t->candidats;
	t->candidats = NULL;
	t->distance_max = HAUTEUR_DEFAUT;
	 
	tgls[0] = triangle_create(t->s[0], t->s[1], v, NULL, t, t->v[2]);
	tgls[1] = triangle_create(v, t->s[1], t->s[2], t->v[0], t, tgls[0]);
	tgls[0]->v[0] = tgls[1];
	
	tgls[2] = t;
	t->s[1] = v;
	t->v[0] = tgls[1];	t->v[2] = tgls[0]; 	//t->v[1] = idem
	
	//actualiser voisins des triangles voisins
	triangle* voisin = tgls[0]->v[2];
	int indice;
	if(voisin != NULL)
	{
		indice = triangle_indice_voisin(voisin, t);
		voisin->v[indice] = tgls[0];
	}
	
	voisin = tgls[1]->v[0];
	if(voisin != NULL)
	{
		indice = triangle_indice_voisin(voisin, t);
		voisin->v[indice] = tgls[1];
	}
	
	//remettre les candidat dans le bon triangle
	v = v->link[VLINK_CANDIDAT][VLINK_FORWARD];
	repartageCandidats(tgls, 3, v);
	
	pqueue_enqueue(pq, tgls[0]);
	pqueue_enqueue(pq, tgls[1]);
	pqueue_enqueue(pq, tgls[2]);
	//pqueue_update_triangle(pq,t);
	
	tstack_push(pile, tgls[0]);
	tstack_push(pile, tgls[1]);
	tstack_push(pile, tgls[2]);
	free(tgls);
}




void corrigeTriangles(tstack* pile, vertex* p, pqueue* pq)
{
	while(pile->nb != 0)
	{
		triangle* t1 = tstack_pop(pile);
		
		int i1 = triangle_indice_point(t1, p);
		
		triangle* t2 = t1->v[i1];
		if(t2 == NULL)
			continue;
			
		int i2 = triangle_indice_voisin(t2, t1);
		vertex* p2 = t2->s[i2];
		
		//il ne faut pas que le quadrilatère formé par les deux triangles soit non-convexe (avec un angle de plus de 180°)

		if(!triangleInCircle(t1, p2))
			continue;
		vertex* candid = t1->candidats, *candid2 = t2->candidats;
		t1->candidats = NULL;
		t2->candidats = NULL;
		t1->distance_max = HAUTEUR_DEFAUT;
		t2->distance_max = HAUTEUR_DEFAUT;
		//on fait tourner les indices de triangle pour que ce soit plus facile après, avec 0 qui sera le point opposé de chaque côté.
		rotationIndiceTriangle(t1, i1);
		rotationIndiceTriangle(t2, i2);
		
		t1->s[2] = t2->s[0];
		t1->v[0] = t2->v[1];			//t1->v[2] idem
		
		t2->s[2] = t1->s[0];
		t2->v[0] = t1->v[1];			//t2->v[2] idem;

		//actualiser voisins des triangles voisins
		triangle* voisin = t1->v[0];
		int indice;
		if(voisin != NULL)		{
			indice = triangle_indice_voisin(voisin, t2);
			voisin->v[indice] = t1;
		}
		voisin = t2->v[0];
		if(voisin != NULL)		{
			indice = triangle_indice_voisin(voisin, t1);
			voisin->v[indice] = t2;
		}
	
		
		t1->v[1] = t2;
		t2->v[1] = t1;
		
		triangle** tgls = (triangle**) malloc(sizeof(triangle*)*2);
		tgls[0] = t1;
		tgls[1] = t2;
		repartageCandidats(tgls, 2, candid);
		repartageCandidats(tgls, 2, candid2);
		free(tgls);
		tstack_push(pile, t1);
		tstack_push(pile, t2);
		pqueue_update_triangle(pq,t1);
		pqueue_update_triangle(pq,t2);
		pqueue_update_triangle(pq,t1);
	}	
}
