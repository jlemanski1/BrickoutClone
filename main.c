#include <epoxy/gl.h>
#include <epoxy/glx.h>
#include <gtk-3.0/gtk/gtk.h>
#include <math.h>
#include "DashGL/dashgl.h"

#define WIDTH 640.0f
#define HEIGHT 480.0f

// Function Prototypes
static void on_realize(GtkGLArea*area);
static void on_render(GtkGLArea *area, GdkGLContext *context);
static gboolean on_idle(gpointer data);
static gboolean on_keydown(GtkWidget *widget, GdkEventKey *event);
static gboolean on_keyup(GtkWidget *widget, GdkEventKey *event);

// Ball
struct {
    float dx, dy;   // delta x, delta y
    float r;        // Radius
    vec3 pos;     // Position
    vec3 colour;  // Colour
    mat4 mvp;     
    GLuint vbo;
    gboolean left, right, top, bottom;
} ball;

// Paddle
struct {
    float width;    // Paddle width
    float height;   // Paddle Height
    float dx;       // delta x
    vec3 pos;     // Position
    vec3 colour;  // Colour
    mat4 mvp;
    GLuint vbo;
    gboolean key_left;
    gboolean key_right;
} paddle;

GLuint program;
GLuint vao;
GLint attribute_coord2d;
GLint uniform_mvp, uniform_colour;

int main(int argc, char *argv[]) {
    GtkWidget *window;
    GtkWidget *glArea;

    gtk_init(&argc,&argv);

    // Initialize Window
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Brickout Clone");
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_window_set_default_size(GTK_WINDOW(window), WIDTH, HEIGHT);
    gtk_window_set_type_hint(GTK_WINDOW(window), GDK_WINDOW_TYPE_HINT_UTILITY);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(window, "key-press-event", G_CALLBACK(on_keydown), NULL);
    g_signal_connect(window, "key-release-event", G_CALLBACK(on_keyup), NULL);

    // Initialize GTK GL Area
	glArea = gtk_gl_area_new();
	gtk_widget_set_vexpand(glArea, TRUE);
	gtk_widget_set_hexpand(glArea, TRUE);
	g_signal_connect(glArea, "realize", G_CALLBACK(on_realize), NULL);
	g_signal_connect(glArea, "render", G_CALLBACK(on_render), NULL);
	gtk_container_add(GTK_CONTAINER(window), glArea);

    // Show widgets
    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}


static void on_realize(GtkGLArea *area) {

	// Debug Message
	g_print("on realize\n");

    // Set gtk glArea and handle unexpected errors
	gtk_gl_area_make_current(area);
	if(gtk_gl_area_get_error(area) != NULL) {
		fprintf(stderr, "ERROR: gtk_gl_area_make_current(area) failed\n");
		return;
	}

	const GLubyte *renderer = glGetString(GL_RENDER);
	const GLubyte *version = glGetString(GL_VERSION);
	const GLubyte *shader = glGetString(GL_SHADING_LANGUAGE_VERSION);

    // Display shader, renderer, and openGL versions
	printf("Shader %s\n", shader);
	printf("Renderer: %s\n", renderer);
	printf("OpenGL version supported %s\n", version);

	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

    ball.r = 18.0f;     // Set ball's radius

	int i;
	float angle, nextAngle;
	int num_segments = 99;

	GLfloat circle_vertices[6 * 100];
	
	for(i = 0; i < num_segments; i++) {

		angle = i * 2.0f * M_PI / (num_segments - 1);
		nextAngle = (i+1) * 2.0f * M_PI / (num_segments - 1);

		circle_vertices[i*6 + 0] = cos(angle) * ball.r;
		circle_vertices[i*6 + 1] = sin(angle) * ball.r;

		circle_vertices[i*6 + 2] = cos(nextAngle) * ball.r;
		circle_vertices[i*6 + 3] = sin(nextAngle) * ball.r;

		circle_vertices[i*6 + 4] = 0.0f;
		circle_vertices[i*6 + 5] = 0.0f;

	}

	glGenBuffers(1, &ball.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, ball.vbo);
	glBufferData(
		GL_ARRAY_BUFFER,
		sizeof(circle_vertices),
		circle_vertices,
		GL_STATIC_DRAW
	);

    // Set paddle dimensions
    paddle.width = 50.0f;
    paddle.height = 7.0f;

    // Set paddle vertices
	GLfloat paddle_vertices[] = {
		-paddle.width, -paddle.height,
		-paddle.width,  paddle.height,
		 paddle.width,  paddle.height,
		 paddle.width,  paddle.height,
		 paddle.width, -paddle.height,
		-paddle.width, -paddle.height
	};

	glGenBuffers(1, &paddle.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, paddle.vbo);
	glBufferData(
		GL_ARRAY_BUFFER,
		sizeof(paddle_vertices),
		paddle_vertices,
		GL_STATIC_DRAW
	);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);
	glDisableVertexAttribArray(0);
	
    // Declare pointer to shaders
	const char *vs = "shaders/vertex.glsl";
	const char *fs = "shaders/fragment.glsl";

	program = shader_load_program(vs, fs);  // Load shader

	const char *attribute_name = "coord2d";
	attribute_coord2d = glGetAttribLocation(program, attribute_name);
	if(attribute_coord2d == -1) {
		fprintf(stderr, "Could not bind attribute %s\n", attribute_name);
		return;
	}

    // Orthogonal Matrix
	const char *uniform_name = "orthograph";
	GLint uniform_ortho = glGetUniformLocation(program, uniform_name);
	if(uniform_ortho == -1) {
		fprintf(stderr, "Could not bind uniform %s\n", uniform_name);
		return;
	}
	
    // Model Position Matrix
	uniform_name = "mvp";
	uniform_mvp = glGetUniformLocation(program, uniform_name);
	if(uniform_mvp == -1) {
		fprintf(stderr, "Could not bind uniform %s\n", uniform_name);
		return;
	}

    // Colour Matrix
    uniform_name = "colour";
    uniform_colour = glGetUniformLocation(program, uniform_name);
    if (uniform_colour == -1) {
        fprintf(stderr, "Could not bind uniform %s\n", uniform_name);
        return;
    }

	glUseProgram(program);

	mat4 orthograph;
	mat4_orthagonal(WIDTH, HEIGHT, orthograph);

	glUniformMatrix4fv(uniform_ortho, 1, GL_FALSE, orthograph);
	g_timeout_add(20, on_idle, (void*)area);
	
    // Set ball's speed
	ball.dx = 2.0f;
	ball.dy = 3.0f;
    // Set ball position
    ball.pos[0] = 50.0f;
    ball.pos[1] = 50.0f;
    ball.pos[2] = 0.0f;
    // Set ball colour
    ball.colour[0] = 0.0f;
    ball.colour[1] = 1.0f;
    ball.colour[2] = 0.0f;

    // Set Paddle's speed
    paddle.dx = 3.0f;
    // Set Paddle position
    paddle.pos[0] = WIDTH / 2.0f;
    paddle.pos[1] = 20.0f;
    paddle.pos[2] = 0.0f;
    // Set paddle colour
    paddle.colour[0] = 0.0f;
    paddle.colour[1] = 0.0f;
    paddle.colour[2] = 0.0f;

    //Disable paddle movement
    paddle.key_left = FALSE;
    paddle.key_right = FALSE;
}


static void on_render(GtkGLArea *area, GdkGLContext *context) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	mat4_translate(ball.pos, ball.mvp);
	glUniformMatrix4fv(uniform_mvp, 1, GL_FALSE, ball.mvp);
    glUniform3fv(uniform_colour, 1, ball.colour);

	glBindVertexArray(vao);
	glEnableVertexAttribArray(attribute_coord2d);

	glBindBuffer(GL_ARRAY_BUFFER, ball.vbo);
	glVertexAttribPointer(
		attribute_coord2d,
		2,
		GL_FLOAT,
		GL_FALSE,
		0,
		0
	);

	glDrawArrays(GL_TRIANGLES, 0, 3 * 100);

    mat4_translate(paddle.pos, paddle.mvp);
    glUniformMatrix4fv(uniform_mvp, 1, GL_FALSE, paddle.mvp);
    glUniform3fv(uniform_colour, 1, paddle.colour);

    glBindBuffer(GL_ARRAY_BUFFER, paddle.vbo);
    glVertexAttribPointer(
        attribute_coord2d,
        2,
        GL_FLOAT,
        GL_FALSE,
        0,
        0
    );

    glDrawArrays(GL_TRIANGLES, 0, 3 * 2);
	glDisableVertexAttribArray(attribute_coord2d);
}

static gboolean on_idle(gpointer data) {
    // Start ball moving
    ball.pos[0] += ball.dx;
    ball.pos[1] += ball.dy;

    // Ball-Window Collision
    if (ball.pos[0] > WIDTH) {
        ball.pos[0] = WIDTH;
        ball.dx *= -1;
    } else if (ball.pos[0] < 0) {
        ball.pos[0] = 0;
        ball.dx *= -1;
    }
    if (ball.pos[1] > HEIGHT) {
        ball.pos[1] = HEIGHT;
        ball.dy *= -1;
    } else if (ball.pos[1] < 0) {
        ball.pos[1] = 0;
        ball.dy *= -1;
    }

    // Keyboard Input
    if(paddle.key_left) {
		paddle.pos[0] -= paddle.dx;
	}

	if(paddle.key_right) {
		paddle.pos[0] += paddle.dx;
	}

    // Paddle-Window Collision
	if(paddle.pos[0] < 0) {
		paddle.pos[0] = 0.0f;
	} else if(paddle.pos[0] > WIDTH) {
		paddle.pos[0] = WIDTH;
	}

    // Paddle-Ball Collision
    ball.left = ball.pos[0] > paddle.pos[0] - paddle.width;
    ball.right = ball.pos[0] < paddle.pos[0] + paddle.width;
	ball.top = ball.pos[1] < paddle.pos[1] + paddle.height;
	ball.bottom = ball.pos[1] > paddle.pos[1] - paddle.height;

	if(ball.dy < 0 && ball.left && ball.right && ball.top && ball.bottom) {
		ball.dy *= -1.05;
	}

    gtk_widget_queue_draw(GTK_WIDGET(data));
    return TRUE;
}

/*
    Purpose:    Called when a key is pressed
    Params:
    Returns:
*/
static gboolean on_keydown(GtkWidget *widget, GdkEventKey *event) {
    switch (event->keyval)
    {
        // Left key down
        case GDK_KEY_Left:
            paddle.key_left = TRUE;
            break;
        // Right key down
        case GDK_KEY_Right:
            paddle.key_right = TRUE;
            break;
    }
}


/*
    Purpose:    Called when a key is lifted
    Params:
    Returns:
*/
static gboolean on_keyup(GtkWidget *widget, GdkEventKey *event) {

	switch(event->keyval) {
        // Left Key up
		case GDK_KEY_Left:
			paddle.key_left = FALSE;
		break;
        // Right key up
		case GDK_KEY_Right:
			paddle.key_right = FALSE;
		break;
	}
}
