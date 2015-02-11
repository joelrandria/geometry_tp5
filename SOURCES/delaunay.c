#include "delaunay.h"
#include "rendering.h"
#include "construction.h"

#ifdef __LINUX__
#include <GL/glut.h>
#elif __APPLE__
#include <GLUT/glut.h>
#endif

#include <time.h>
#include <unistd.h>
#include <stdio.h>

/*! Initialisation des variables externes. */
pqueue* _queue = NULL;
tstack* _stack = NULL;
settings* _settings = NULL;

double myRandom(double a, double b)
{
  double tmp = random();
  return (a + tmp * ((b - a) / RAND_MAX));
}

vertex* create_random_points(settings* s)
{
  const int naturel = VLINK_NATURAL, lexico = VLINK_LEXICO, suiv = VLINK_FORWARD;

  // Vertices du carré ]0,1[² dans l'ordre lexicographique
  vertex* v = vertex_create(0, 0, /* myRandom(0, s->altitude_max) */0);
  vertex* premier = v;
  vertex* vPrec = v;

  v = vertex_create(0, 1, /* myRandom(0, s->altitude_max) */0);
  vPrec->link[naturel][suiv] = v;
  vPrec->link[lexico][suiv] = v;
  vPrec = v;

  v = vertex_create(1, 0, /* myRandom(0, s->altitude_max) */0);
  vPrec->link[naturel][suiv] = v;
  vPrec->link[lexico][suiv] = v;
  vPrec = v;

  v = vertex_create(1, 1, /* myRandom(0, s->altitude_max) */0);
  vPrec->link[naturel][suiv] = v;
  vPrec->link[lexico][suiv] = v;
  vertex* dernier = v;

  int i;
  for (i = 4; i < _settings->vertex_count; ++i)
  {
      v = vertex_create(myRandom(0, 1), myRandom(0, 1), myRandom(0, s->altitude_max));

    dernier->link[naturel][suiv] = v;
    dernier = v;
  }

  printf("fin de création des points.\n");

  return premier;
}

#define MIN(a, b)	(a < b	?	a	:	b)

int main(int argc, char **argv)
{
  clock_t start = clock();

  _settings = settings_alloc();
  settings_from_cmdline(argc, argv, _settings);

  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA | GLUT_DEPTH);

  glutInitWindowPosition(5,5);
  glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
  glutCreateWindow("Delaunay");

  glutIdleFunc(on_idle_event);
  glutDisplayFunc(render);
  glutKeyboardFunc(process_key_pressed);
  glutSpecialFunc(process_specialkey_pressed);
  glutMotionFunc(process_mouse_moved);

  vertex* premier = create_random_points(_settings);

	//crée une priority_queue et une pile de capacités suffisantes
	const int taille2 = (2 * _settings->vertex_count - 6);
	const int taille = (_settings->stop_mode == STOPMODE_FACE_COUNT ? 
							MIN(taille2, _settings->max_face_count+3)	:							taille2);
	_queue = pqueue_create(taille);
	_stack = tstack_create(taille);

  initCarre(premier, _queue);

	int nbIteration = 0;
	while(insertPoint(_queue, _stack, _settings))
		nbIteration++;

  glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
  glClearColor(0, 0, 0, 0);
	if(_settings->view_mode == VIEWMODE_3D)
		glEnable(GL_DEPTH_TEST);

  printf("fin de génération de la triangulation!\n\tTemps : %lf s\n\tNombre de face : %d\n\tNombre de candidats insérés : %d\n\tTaille max atteinte dans la pile : %d\n", (clock()-start)/((double)CLOCKS_PER_SEC), _queue->size, nbIteration,_stack->nbMaxAtteint );

  if (_settings->view_mode != VIEWMODE_CONSOLE)
    glutMainLoop();

  vertex_delete(premier, VLINK_NATURAL);
  pqueue_delete(_queue);
  tstack_delete(_stack);

  return EXIT_SUCCESS;
}

void on_idle_event()
{
  clock_t start = clock();
  render();
  clock_t end = clock();

  if((end-start)/((double)CLOCKS_PER_SEC) < 0.020)	//60 FPS
    usleep(20000.0 - 1000000.0 * (end - start) / ((double)CLOCKS_PER_SEC));
}
