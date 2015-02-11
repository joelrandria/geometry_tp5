#include "rendering.h"
#include "delaunay.h"
#include "vertex.h"
#include "camera.h"

#ifdef __LINUX__
#include <GL/glut.h>
#elif __APPLE__
#include <GLUT/glut.h>
#endif

#include <math.h>
#include <stdio.h>

/// Caméra globale
static camera* _camera = NULL;

/*! Valeurs et constantes relatives aux déplacement de la caméra */
static vec3f _camera_front_move = {0, 0, -0.05f};
static vec3f _camera_back_move = {0, 0, 0.05f};
static vec3f _camera_right_move = {0.05f, 0, 0};
static vec3f _camera_left_move = {-0.05f, 0, 0};
static vec3f _camera_up_move = {0, 0.05f, 0};
static vec3f _camera_down_move = {0, -0.05f, 0};

static const float _camera_degree_rotamount = 0.5f;

/**
 * Initialise la caméra globale si celle-ci ne l'est pas encore.
 * Affecte à celle-ci une position, une direction up et une direction front par défaut.
 */
static void camera_initialize()
{
  vec3f camera_pos;
  vec3f camera_up;
  vec3f camera_front;

  if (_camera == NULL)
  {
    camera_pos.x = 0.5f;
    camera_pos.y = 0.5f;
    camera_pos.z = 1.85f;

    camera_up.x = 0;
    camera_up.y = 1;
    camera_up.z = 0;

    camera_front.x = 0;
    camera_front.y = 0;
    camera_front.z = -1;

    /* camera_pos.x = 0.5f; */
    /* camera_pos.y = -0.5f; */
    /* camera_pos.z = 0.5f; */

    /* camera_up.x = 0; */
    /* camera_up.y = 0; */
    /* camera_up.z = 1; */

    /* camera_front.x = 0; */
    /* camera_front.y = 1; */
    /* camera_front.z = 0; */

    _camera = camera_create(&camera_pos, &camera_up, &camera_front);
  }
}
/**
 * Transtype la matrice 4x4 de passage du repère monde au repère caméra en une matrice 4x4 compatible OpenGL 2.
 *
 * @param c La caméra.
 * @param m Un tableau de 16 valeurs GLdouble recevant la matrice 4x4.
 */
static void camera_get_modelview_matrix(camera* c, GLdouble m[16])
{
  m[0] = c->world_to_view.v[0][0];
  m[1] = c->world_to_view.v[1][0];
  m[2] = c->world_to_view.v[2][0];
  m[3] = c->world_to_view.v[3][0];

  m[4] = c->world_to_view.v[0][1];
  m[5] = c->world_to_view.v[1][1];
  m[6] = c->world_to_view.v[2][1];
  m[7] = c->world_to_view.v[3][1];

  m[8] = c->world_to_view.v[0][2];
  m[9] = c->world_to_view.v[1][2];
  m[10] = c->world_to_view.v[2][2];
  m[11] = c->world_to_view.v[3][2];

  m[12] = c->world_to_view.v[0][3];
  m[13] = c->world_to_view.v[1][3];
  m[14] = c->world_to_view.v[2][3];
  m[15] = c->world_to_view.v[3][3];
}

/**
 * Effectue le rendu 2D fil-de-fer en vue de dessus du terrain.
 *
 * @param p La file de priorité contenant l'ensemble des faces du terrain à afficher.
 */
static void render_2d(pqueue* p)
{
  int i, j;
  triangle* t;

  glMatrixMode(GL_PROJECTION);
  gluOrtho2D(0, 1, 0, 1);

	vertex* v;
	float val;
	for (i = 1; i <= p->size; ++i)
	{
		t = p->items[i];
		if(!_settings->afficheBordure)
			if(	t->s[0]->X == 0.0 || t->s[1]->X == 0.0 || t->s[2]->X == 0.0 ||
				t->s[0]->X == 1.0 || t->s[1]->X == 1.0 || t->s[2]->X == 1.0)
					continue;
		glBegin(GL_LINE_LOOP);
		for (j = 0; j < 3; ++j)
		{
			v = t->s[j];
			val = v->Z/_settings->altitude_max;		//(v->Z-_altitude_min)/(_altitude_max-_altitude_min);
			//glColor3f(0.625+val*0.375, 0.25+val*0.75, val);
			glColor3f(0.600f+val*0.40f, 0.20f+val*0.80f, val);
			glVertex2f(v->X, v->Y);
		}
		glEnd();
	}
}

/**
 * Effectue le rendu 3D du terrain.
 *
 * @param p La file de priorité contenant l'ensemble des faces du terrain à afficher.
 */
static void render_3d(pqueue* p)
{
  int i, j;

  triangle* t;
  vertex* v;
  float val;

  GLdouble view_matrix[16];

  camera_initialize();

  glEnable(GL_DEPTH_TEST);

  glMatrixMode(GL_MODELVIEW);
  camera_get_modelview_matrix(_camera, view_matrix);
  glLoadMatrixd(view_matrix);

  glMatrixMode(GL_PROJECTION);
  gluPerspective(60.f, 1.f, 0.01f, 10.f);

	for (i = 1; i <= p->size; ++i)
	{
		t = p->items[i];

		if(!_settings->afficheBordure)
			if(	t->s[0]->X == 0.0 || t->s[1]->X == 0.0 || t->s[2]->X == 0.0 ||
				t->s[0]->X == 1.0 || t->s[1]->X == 1.0 || t->s[2]->X == 1.0)
					continue;
		
		glBegin(GL_TRIANGLES);
		for (j = 0; j < 3; ++j)
		{
			v = t->s[j];
			val = v->Z/_settings->altitude_max;
			glColor3f(0.600f+val*0.40f, 0.20f+val*0.80f, val);
			glVertex3f(v->X, v->Y, v->Z);
		}
		glEnd();
		
		if(_settings->afficheLine)
		{
			glColor3f(0.5f, 0.5f, 0.5f);
			glBegin(GL_LINE_LOOP);
			for (j = 0; j < 3; ++j)
			{
				v = t->s[j];
				glVertex3f(v->X, v->Y, v->Z+0.001);
			}
			glEnd();
		}
	}
}

void render()
{
  glClearColor(0, 0, 0, 0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  switch (_settings->view_mode)
  {
  case VIEWMODE_2D: render_2d(_queue); break;
  case VIEWMODE_3D: render_3d(_queue); break;
  default: break;
  }

  glFlush();
}

void process_key_pressed(unsigned char key, int x, int y)
{
  vec3f* t;

  t = 0;

  switch (key)
  {
  case 'z': t = &_camera_front_move; break;
  case 's': t = &_camera_back_move; break;
  case 'q': t = &_camera_left_move; break;
  case 'd': t = &_camera_right_move; break;

  case 'p': camera_print(_camera); break;

  case '+': glEnable(GL_CULL_FACE); break;
  case '-': glDisable(GL_CULL_FACE); break;

  case '*': glEnable(GL_DEPTH_TEST); printf("GL_DEPTH_TEST enabled\r\n"); break;
  case '/': glDisable(GL_DEPTH_TEST); printf("GL_DEPTH_TEST disabled\r\n"); break;

  case ' ': _settings->view_mode = (_settings->view_mode == VIEWMODE_3D ? VIEWMODE_2D : VIEWMODE_3D); break;
  case 'b': _settings->afficheBordure = !_settings->afficheBordure;	break;
  case 'l': _settings->afficheLine = !_settings->afficheLine;	break;
  }

  if (t != 0)
    camera_translate(_camera, t);
}
void process_specialkey_pressed(int key, int x, int y)
{
  vec3f* t;

  t = 0;

  switch (key)
  {
  case GLUT_KEY_PAGE_UP:
  case GLUT_KEY_UP:		t = &_camera_up_move; break;
  case GLUT_KEY_PAGE_DOWN:
  case GLUT_KEY_DOWN:		t = &_camera_down_move; break;
  case GLUT_KEY_LEFT: 		t = &_camera_left_move; break;
  case GLUT_KEY_RIGHT: 		t = &_camera_right_move; break;
  }

  if (t != 0)
    camera_translate(_camera, t);
}
void process_mouse_moved(int x, int y)
{
  int diff_x;
  int diff_y;

  static int last_x = -1;
  static int last_y = -1;

  if (last_x >= 0 && last_y >= 0)
  {
    diff_x = x - last_x;
    diff_y = y - last_y;

    if (fabs(diff_x) < 20 && fabs(diff_y) < 20)
    {
      camera_yaw(_camera, -diff_x * _camera_degree_rotamount);
      camera_pitch(_camera, -diff_y * _camera_degree_rotamount);
    }
  }

  last_x = x;
  last_y = y;
}
