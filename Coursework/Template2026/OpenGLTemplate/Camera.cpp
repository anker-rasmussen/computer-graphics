#include "Camera.h"
#include "GameWindow.h"
#include <GLFW/glfw3.h>

// Constructor for camera -- initialise with some default values
CCamera::CCamera()
{
	m_position = glm::vec3(0.0f, 10.0f, 100.0f);
	m_view = glm::vec3(100.0f, 0.0f, 0.0f);
	m_upVector = glm::vec3(0.0f, 1.0f, 0.0f);
	m_speed = 0.025f;
}
CCamera::~CCamera()
{}

// Set the camera at a specific position, looking at the view point, with a given up vector
void CCamera::Set(const glm::vec3 &position, const glm::vec3 &viewpoint, const glm::vec3 &upVector)
{
	m_position = position;
	m_view = viewpoint;
	m_upVector = upVector;

}

// Respond to mouse movement
void CCamera::SetViewByMouse()
{
	GLFWwindow* window = GameWindow::GetInstance().GetWindow();

	double mouseX, mouseY;
	glfwGetCursorPos(window, &mouseX, &mouseY);

	static bool firstMouse = true;
	static double lastX = 0.0, lastY = 0.0;

	if (firstMouse) {
		lastX = mouseX;
		lastY = mouseY;
		firstMouse = false;
		return;
	}

	float angle_y = (float)(lastX - mouseX) / 10000.0f;
	float angle_z = (float)(lastY - mouseY) / 10000.0f;

	lastX = mouseX;
	lastY = mouseY;

	static float rotation_x = 0.0f;
	rotation_x -= angle_z;

	float maxAngle = 1.56f; // Just a little bit below PI / 2

	if (rotation_x > maxAngle) {
		rotation_x = maxAngle;
	} else if (rotation_x < -maxAngle) {
		rotation_x = -maxAngle;
	} else {
		glm::vec3 cross = glm::cross(m_view - m_position, m_upVector);
		glm::vec3 axis = glm::normalize(cross);

		RotateViewPoint(angle_z, axis);
	}

	RotateViewPoint(angle_y, glm::vec3(0, 1, 0));
}

// Rotate the camera view point -- this effectively rotates the camera since it is looking at the view point
void CCamera::RotateViewPoint(float fAngle, const glm::vec3 &vPoint)
{
	glm::vec3 vView = m_view - m_position;

	glm::mat4 R = glm::rotate(glm::mat4(1), fAngle * 180.0f / (float) M_PI, vPoint);
	glm::vec4 newView = R * glm::vec4(vView, 1);

	m_view = m_position + glm::vec3(newView);
}

// Strafe the camera (side to side motion)
void CCamera::Strafe(double direction)
{
	float speed = (float) (m_speed * direction);

	m_position.x = m_position.x + m_strafeVector.x * speed;
	m_position.z = m_position.z + m_strafeVector.z * speed;

	m_view.x = m_view.x + m_strafeVector.x * speed;
	m_view.z = m_view.z + m_strafeVector.z * speed;
}

// Advance the camera (forward / backward motion)
void CCamera::Advance(double direction)
{
	float speed = (float) (m_speed * direction);

	glm::vec3 view = glm::normalize(m_view - m_position);
	m_position = m_position + view * speed;
	m_view = m_view + view * speed;

}

// Update the camera to respond to mouse motion for rotations and keyboard for translation
void CCamera::Update(double dt)
{
	glm::vec3 vector = glm::cross(m_view - m_position, m_upVector);
	m_strafeVector = glm::normalize(vector);

	SetViewByMouse();
	TranslateByKeyboard(dt);
}

// Update the camera to respond to key presses for translation
void CCamera::TranslateByKeyboard(double dt)
{
	GLFWwindow* window = GameWindow::GetInstance().GetWindow();

	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		Advance(1.0*dt);
	}

	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		Advance(-1.0*dt);
	}

	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		Strafe(-1.0*dt);
	}

	if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		Strafe(1.0*dt);
	}
}
// Return the camera position
glm::vec3 CCamera::GetPosition() const
{
	return m_position;
}

// Return the camera view point
glm::vec3 CCamera::GetView() const
{
	return m_view;
}

// Return the camera up vector
glm::vec3 CCamera::GetUpVector() const
{
	return m_upVector;
}

// Return the camera strafe vector
glm::vec3 CCamera::GetStrafeVector() const
{
	return m_strafeVector;
}

// Return the camera perspective projection matrix
glm::mat4* CCamera::GetPerspectiveProjectionMatrix()
{
	return &m_perspectiveProjectionMatrix;
}

// Return the camera orthographic projection matrix
glm::mat4* CCamera::GetOrthographicProjectionMatrix()
{
	return &m_orthographicProjectionMatrix;
}

// Set the camera perspective projection matrix to produce a view frustum with a specific field of view, aspect ratio,
// and near / far clipping planes
void CCamera::SetPerspectiveProjectionMatrix(float fov, float aspectRatio, float nearClippingPlane, float farClippingPlane)
{
	m_perspectiveProjectionMatrix = glm::perspective(fov, aspectRatio, nearClippingPlane, farClippingPlane);
}

// The the camera orthographic projection matrix to match the width and height passed in
void CCamera::SetOrthographicProjectionMatrix(int width, int height)
{
	m_orthographicProjectionMatrix = glm::ortho(0.0f, float(width), 0.0f, float(height), -1.0f, 1.0f);
}

// Get the camera view matrix
glm::mat4 CCamera::GetViewMatrix()
{
	return glm::lookAt(m_position, m_view, m_upVector);
}

// The normal matrix is used to transform normals to eye coordinates -- part of lighting calculations
glm::mat3 CCamera::ComputeNormalMatrix(const glm::mat4 &modelViewMatrix)
{
	return glm::transpose(glm::inverse(glm::mat3(modelViewMatrix)));
}
