/* ============================================================================
 *  Trabalho de Computacao Grafica - Transformacoes Geometricas 2D
 *  ------------------------------------------------------------------------
 *  Desenho: casa com telhado, porta, janelas, chamine e sol.
 *  Recursos:
 *    - Translacao, escala, rotacao e reflexao
 *    - Duas abordagens comutaveis:
 *        (a) funcoes internas do OpenGL (glTranslatef / glScalef / glRotatef)
 *        (b) funcoes proprias (matrizes 3x3 em coordenadas homogeneas)
 *    - Composicao em ordem controlada (permite testar ordem das transformacoes)
 *    - Reset para o estado inicial
 *  Compilar (Linux): g++ main.cpp -o trab -lGL -lGLU -lglut
 * ==========================================================================*/

#include <GL/glut.h>
#include <cmath>
#include <cstdio>
#include <vector>

/* ============================================================================
 *  1) MATRIZES 3x3 EM COORDENADAS HOMOGENEAS (2D)
 * ==========================================================================*/

struct Mat3 {
    float m[3][3];
};

static Mat3 matIdentity() {
    Mat3 r = {{{1,0,0},{0,1,0},{0,0,1}}};
    return r;
}

/* Produto matricial C = A * B */
static Mat3 matMul(const Mat3& a, const Mat3& b) {
    Mat3 r;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j) {
            r.m[i][j] = 0.0f;
            for (int k = 0; k < 3; ++k)
                r.m[i][j] += a.m[i][k] * b.m[k][j];
        }
    return r;
}

/* Translacao:  [1 0 tx]
 *              [0 1 ty]
 *              [0 0  1]  */
static Mat3 matTranslate(float tx, float ty) {
    Mat3 r = matIdentity();
    r.m[0][2] = tx;
    r.m[1][2] = ty;
    return r;
}

/* Escala:      [sx 0 0]
 *              [0 sy 0]
 *              [0  0 1]  */
static Mat3 matScale(float sx, float sy) {
    Mat3 r = matIdentity();
    r.m[0][0] = sx;
    r.m[1][1] = sy;
    return r;
}

/* Rotacao antihoraria em torno da origem:
 *   [cos -sin 0]
 *   [sin  cos 0]
 *   [ 0    0  1]  */
static Mat3 matRotate(float degrees) {
    const float PI = 3.14159265358979f;
    float rad = degrees * PI / 180.0f;
    float c = std::cos(rad), s = std::sin(rad);
    Mat3 r = matIdentity();
    r.m[0][0] =  c; r.m[0][1] = -s;
    r.m[1][0] =  s; r.m[1][1] =  c;
    return r;
}

/* ============================================================================
 *  2) OPERACOES (lista ordenada)
 * ==========================================================================*/

enum OpType {
    OP_TRANSLATE,
    OP_SCALE,
    OP_ROTATE,
    OP_REFLECT_X,   // reflexao em relacao ao eixo X  ->  (x, y) -> (x, -y)
    OP_REFLECT_Y    // reflexao em relacao ao eixo Y  ->  (x, y) -> (-x, y)
};

struct Op {
    OpType type;
    float  a, b;    // parametros (ex.: tx,ty | sx,sy | angulo)
};

static std::vector<Op> g_ops;        // pilha de operacoes, na ordem aplicada
static bool            g_customMode = false;  // false = OpenGL, true = proprio
static Mat3            g_matrix;              // matriz atual (modo proprio)

static Mat3 opToMatrix(const Op& op) {
    switch (op.type) {
        case OP_TRANSLATE: return matTranslate(op.a, op.b);
        case OP_SCALE:     return matScale(op.a, op.b);
        case OP_ROTATE:    return matRotate(op.a);
        case OP_REFLECT_X: return matScale(1.0f, -1.0f);
        case OP_REFLECT_Y: return matScale(-1.0f, 1.0f);
    }
    return matIdentity();
}

/* Aplica a operacao na matriz MODELVIEW corrente do OpenGL.
 * glTranslatef/glScalef/glRotatef POST-multiplicam M <- M * Op,
 * logo a ordem das chamadas aqui corresponde exatamente a ordem
 * em que multiplicamos as matrizes no modo proprio. */
static void applyOpGL(const Op& op) {
    switch (op.type) {
        case OP_TRANSLATE: glTranslatef(op.a, op.b, 0.0f);           break;
        case OP_SCALE:     glScalef(op.a, op.b, 1.0f);               break;
        case OP_ROTATE:    glRotatef(op.a, 0.0f, 0.0f, 1.0f);        break;
        case OP_REFLECT_X: glScalef(1.0f, -1.0f, 1.0f);              break;
        case OP_REFLECT_Y: glScalef(-1.0f, 1.0f, 1.0f);              break;
    }
}

static void pushOp(OpType t, float a = 0.0f, float b = 0.0f) {
    Op op = { t, a, b };
    g_ops.push_back(op);
}

/* ============================================================================
 *  3) VERTICE (aplica a matriz apenas no modo proprio)
 * ==========================================================================*/

static void vtx(float x, float y) {
    if (g_customMode) {
        /* Multiplicacao [x y 1] * M (linha vetor * matriz) */
        float nx = g_matrix.m[0][0]*x + g_matrix.m[0][1]*y + g_matrix.m[0][2];
        float ny = g_matrix.m[1][0]*x + g_matrix.m[1][1]*y + g_matrix.m[1][2];
        glVertex2f(nx, ny);
    } else {
        /* No modo OpenGL quem transforma e a propria MODELVIEW */
        glVertex2f(x, y);
    }
}

/* ============================================================================
 *  4) DESENHO (casa + sol) - todas as primitivas usam vtx()
 * ==========================================================================*/

static void drawHouse() {
    /* ---- Corpo ---- */
    glColor3f(0.95f, 0.90f, 0.75f);
    glBegin(GL_QUADS);
        vtx(-1.00f, -1.00f); vtx( 1.00f, -1.00f);
        vtx( 1.00f,  1.00f); vtx(-1.00f,  1.00f);
    glEnd();

    /* Contorno do corpo */
    glColor3f(0.20f, 0.20f, 0.20f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
        vtx(-1.00f, -1.00f); vtx( 1.00f, -1.00f);
        vtx( 1.00f,  1.00f); vtx(-1.00f,  1.00f);
    glEnd();

    /* ---- Telhado ---- */
    glColor3f(0.75f, 0.20f, 0.20f);
    glBegin(GL_TRIANGLES);
        vtx(-1.20f, 1.00f); vtx(1.20f, 1.00f); vtx(0.00f, 2.00f);
    glEnd();

    /* ---- Porta ---- */
    glColor3f(0.45f, 0.25f, 0.10f);
    glBegin(GL_QUADS);
        vtx(-0.30f, -1.00f); vtx( 0.30f, -1.00f);
        vtx( 0.30f, -0.30f); vtx(-0.30f, -0.30f);
    glEnd();

    /* ---- Janelas ---- */
    glColor3f(0.55f, 0.80f, 1.00f);
    glBegin(GL_QUADS);
        vtx( 0.40f, 0.20f); vtx( 0.80f, 0.20f);
        vtx( 0.80f, 0.60f); vtx( 0.40f, 0.60f);
        vtx(-0.80f, 0.20f); vtx(-0.40f, 0.20f);
        vtx(-0.40f, 0.60f); vtx(-0.80f, 0.60f);
    glEnd();

    /* ---- Chamine ---- */
    glColor3f(0.55f, 0.35f, 0.25f);
    glBegin(GL_QUADS);
        vtx(0.55f, 1.30f); vtx(0.80f, 1.30f);
        vtx(0.80f, 2.10f); vtx(0.55f, 2.10f);
    glEnd();

    /* ---- Sol (circulo via leque de triangulos) ---- */
    glColor3f(1.00f, 0.85f, 0.10f);
    glBegin(GL_TRIANGLE_FAN);
        vtx(2.00f, 2.00f);            // centro
        for (int i = 0; i <= 24; ++i) {
            float a = 2.0f * 3.14159265f * i / 24.0f;
            vtx(2.00f + 0.35f * std::cos(a),
                2.00f + 0.35f * std::sin(a));
        }
    glEnd();
}

/* Eixos coordenados (sempre sem transformacao) */
static void drawAxes() {
    glLineWidth(1.0f);
    glColor3f(0.75f, 0.75f, 0.75f);
    glBegin(GL_LINES);
        glVertex2f(-20.0f, 0.0f); glVertex2f(20.0f, 0.0f);
        glVertex2f(0.0f, -20.0f); glVertex2f(0.0f, 20.0f);
    glEnd();
}

/* ============================================================================
 *  5) HUD (texto em coordenadas de tela)
 * ==========================================================================*/

static void drawText(float x, float y, const char* s) {
    glRasterPos2f(x, y);
    while (*s) glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *s++);
}

static void drawHUD() {
    int w = glutGet(GLUT_WINDOW_WIDTH);
    int h = glutGet(GLUT_WINDOW_HEIGHT);

    /* Passa a desenhar em pixels de tela */
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    char buf[256];

    /* Cabecalho */
    glColor3f(0.0f, 0.0f, 0.0f);
    sprintf(buf, "Modo: %s   (tecla M alterna)",
            g_customMode ? "IMPLEMENTACAO PROPRIA (matrizes)"
                         : "OPENGL (glTranslatef/glScalef/glRotatef)");
    drawText(10, h - 20, buf);

    sprintf(buf, "Operacoes aplicadas (ordem): %d", (int)g_ops.size());
    drawText(10, h - 40, buf);

    /* Lista de operacoes */
    int y = h - 62;
    for (size_t i = 0; i < g_ops.size() && i < 14; ++i) {
        const Op& op = g_ops[i];
        switch (op.type) {
            case OP_TRANSLATE: sprintf(buf, "%2d) Transladar (%.2f, %.2f)", (int)i+1, op.a, op.b); break;
            case OP_SCALE:     sprintf(buf, "%2d) Escalar    (%.2f, %.2f)", (int)i+1, op.a, op.b); break;
            case OP_ROTATE:    sprintf(buf, "%2d) Rotacionar %.1f graus",   (int)i+1, op.a);      break;
            case OP_REFLECT_X: sprintf(buf, "%2d) Reflexao eixo X",         (int)i+1);            break;
            case OP_REFLECT_Y: sprintf(buf, "%2d) Reflexao eixo Y",         (int)i+1);            break;
        }
        drawText(10, y, buf);
        y -= 18;
    }

    /* Matriz resultante no modo proprio */
    if (g_customMode) {
        int bx = w - 320;
        glColor3f(0.15f, 0.15f, 0.45f);
        drawText(bx, h - 62, "Matriz resultante (3x3):");
        for (int i = 0; i < 3; ++i) {
            sprintf(buf, "[%7.3f  %7.3f  %7.3f]",
                    g_matrix.m[i][0], g_matrix.m[i][1], g_matrix.m[i][2]);
            drawText(bx, h - 82 - i * 18, buf);
        }
    }

    /* Rodape de comandos */
    glColor3f(0.20f, 0.20f, 0.60f);
    drawText(10, 10,
        "T/t: +/-X   Y/y: +/-Y   setas: transladar   S/s: escala   "
        "R/r: rotacao   F: reflex.eixo X   G: reflex.eixo Y   C: resetar   M: modo   ESC: sair");

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

/* ============================================================================
 *  6) DISPLAY
 * ==========================================================================*/

static void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    drawAxes();

    if (g_customMode) {
        /* Monta M = I * Op1 * Op2 * ... * OpN  (mesma ordem do OpenGL) */
        g_matrix = matIdentity();
        for (size_t i = 0; i < g_ops.size(); ++i)
            g_matrix = matMul(g_matrix, opToMatrix(g_ops[i]));

        /* drawHouse() usa vtx() que aplica g_matrix em cada vertice */
        drawHouse();
    } else {
        glPushMatrix();
            for (size_t i = 0; i < g_ops.size(); ++i)
                applyOpGL(g_ops[i]);
            drawHouse();
        glPopMatrix();
    }

    drawHUD();
    glutSwapBuffers();
}

/* ============================================================================
 *  7) RESHAPE
 * ==========================================================================*/

static void reshape(int w, int h) {
    if (h == 0) h = 1;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float aspect = (float)w / (float)h;
    float halfH  = 6.0f;
    float halfW  = halfH * aspect;
    gluOrtho2D(-halfW, halfW, -halfH, halfH);
    glMatrixMode(GL_MODELVIEW);
}

/* ============================================================================
 *  8) TECLADO
 * ==========================================================================*/

static void keyboard(unsigned char key, int /*x*/, int /*y*/) {
    const float step = 0.25f;    // passo de translacao
    const float sUp  = 1.10f;    // fator de escala

    switch (key) {
        /* Translacao */
        case 't': pushOp(OP_TRANSLATE,  step,  0.0f); break;
        case 'T': pushOp(OP_TRANSLATE, -step,  0.0f); break;
        case 'y': pushOp(OP_TRANSLATE,  0.0f,  step); break;
        case 'Y': pushOp(OP_TRANSLATE,  0.0f, -step); break;

        /* Escala */
        case 's': pushOp(OP_SCALE,  sUp,         sUp        ); break;
        case 'S': pushOp(OP_SCALE,  1.0f / sUp,  1.0f / sUp ); break;

        /* Rotacao */
        case 'r': pushOp(OP_ROTATE,  15.0f); break;
        case 'R': pushOp(OP_ROTATE, -15.0f); break;

        /* Reflexoes */
        case 'f': case 'F': pushOp(OP_REFLECT_X); break;   // (x,y)->(x,-y)
        case 'g': case 'G': pushOp(OP_REFLECT_Y); break;   // (x,y)->(-x,y)

        /* Reset e modo */
        case 'c': case 'C': g_ops.clear();                 break;
        case 'm': case 'M': g_customMode = !g_customMode;  break;

        case 27: exit(0);   // ESC
    }
    glutPostRedisplay();
}

static void special(int key, int /*x*/, int /*y*/) {
    const float step = 0.25f;
    switch (key) {
        case GLUT_KEY_LEFT:  pushOp(OP_TRANSLATE, -step,  0.0f); break;
        case GLUT_KEY_RIGHT: pushOp(OP_TRANSLATE,  step,  0.0f); break;
        case GLUT_KEY_UP:    pushOp(OP_TRANSLATE,  0.0f,  step); break;
        case GLUT_KEY_DOWN:  pushOp(OP_TRANSLATE,  0.0f, -step); break;
    }
    glutPostRedisplay();
}

/* ============================================================================
 *  9) MAIN
 * ==========================================================================*/

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(1000, 720);
    glutCreateWindow("Transformacoes Geometricas 2D - OpenGL vs Matrizes Proprias");

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special);

    glutMainLoop();
    return 0;
}