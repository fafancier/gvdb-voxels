
#define GLEW_STATIC
#include <GL/glew.h>
#include <GL/glfw3.h>


#include <iostream>
#include <gvdb.h>

#include <main.h>

void checkGL(const char* msg)
{
	GLenum errCode;
	errCode = glGetError();
	if (errCode != GL_NO_ERROR) {
		printf("%s, GL ERROR: 0x%x\n", msg, errCode);
	}
}

// Is called whenever a key is pressed/released via GLFW
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	std::cout << key << std::endl;
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
}
const GLuint WIDTH = 800, HEIGHT = 600;

int main() {


	std::cout << "Starting GLFW context, OpenGL 3.3" << std::endl;
	// Init GLFW
	glfwInit();
	// Set all the required options for GLFW
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	// Create a GLFWwindow object that we can use for GLFW's functions
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "LearnOpenGL", nullptr, nullptr);
	if (window == nullptr)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	// Set the required callback functions
	glfwSetKeyCallback(window, key_callback);

	// Set this to true so GLEW knows to use a modern approach to retrieving function pointers and extensions
	glewExperimental = GL_TRUE;
	// Initialize GLEW to setup the OpenGL Function pointers
	if (glewInit() != GLEW_OK)
	{
		std::cout << "Failed to initialize GLEW" << std::endl;
		return -1;
	}


	if (false)
	{
		// Define the viewport dimensions
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		glViewport(0, 0, width, height);

		// Game loop
		while (!glfwWindowShouldClose(window))
		{
			// Check if any events have been activiated (key pressed, mouse moved etc.) and call corresponding response functions
			glfwPollEvents();

			// Render
			// Clear the colorbuffer
			glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			// Swap the screen buffers
			glfwSwapBuffers(window);
		}

		// Terminate GLFW, clearing any resources allocated by GLFW.
		glfwTerminate();
	}


	VolumeGVDB	gvdb;
	// Initialize GVDB
	printf("Starting GVDB.\n");
	gvdb.SetDebug(true);
	gvdb.SetVerbose(true);		// enable/disable console output from gvdb
	gvdb.SetCudaDevice(GVDB_DEV_FIRST);
	gvdb.Initialize();
	gvdb.AddPath("../source/shared_assets/");
	gvdb.AddPath(ASSET_PATH);
	gvdb.ValidateOpenGL();
	Vector3DF mem = cudaGetMemUsage();
	std::cout << "used " << mem.x / 1024.0 << " GBs; free " << mem.y / 1024.0 << " GBs;  total " << mem.z / 1024.0 << " GBs " << std::endl;

	printf("Loading polygon model.\n");

	gvdb.getScene()->AddModel("lucy.obj", 1.0, 0, 0, 0);

	Vector3DF	m_pivot;

	m_pivot.Set(0.3f, 0.45f, 0.3f); // This is the center of the polygon model.

	printf("Configure.\n");
	gvdb.Configure(0, 0, 3, 3, 4);
	gvdb.SetChannelDefault(16, 16, 4);

	gvdb.DestroyChannels();
	gvdb.AddChannel(0, T_FLOAT, 1);
	float voxel_size = 0.5f;
	float part_size = 100.0;					// Part size = 100 mm (default)

	// Create a transform	
	Matrix4F xform, m;
	xform.Identity();

	// Complete poly-to-voxel transform: 
	//    X = S(partsize) S(1/voxelsize) Torigin R
	//  (remember, matrices are multiplied left-to-right but applied conceptually right-to-left)
	m.Scale(part_size, part_size, part_size);
	xform *= m;									// 4th) Apply part size 
	m.Scale(1 / voxel_size, 1 / voxel_size, 1 / voxel_size);
	xform *= m;									// 3rd) Apply voxel size (scale by inverse of this)
	m.Translate(m_pivot.x, m_pivot.y, m_pivot.z);	// 2nd) Move part so origin is at bottom corner 
	xform *= m;
	m.RotateZYX(Vector3DF(0, -10, 0));			// 1st) Rotate part about the geometric center
	xform *= m;

	// Set transform for rendering
	// Scale the GVDB grid by voxelsize to render the model in our desired world coordinates
	gvdb.SetTransform(Vector3DF(0, 0, 0), Vector3DF(voxel_size, voxel_size, voxel_size), Vector3DF(0, 0, 0), Vector3DF(0, 0, 0));
	// Poly-to-Voxels
	// Converts polygons-to-voxels using the GPU	
	Model* model = gvdb.getScene()->getModel(0);


	gvdb.CommitGeometry(0);		// Send the polygons to GPU as 

	gvdb.SolidVoxelize(0, model, &xform, 1.0, 0.5);

	gvdb.Measure(true);

	return 0;
}