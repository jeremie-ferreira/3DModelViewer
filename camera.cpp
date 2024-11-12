#include "camera.h"
#include <iostream>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/ext/matrix_clip_space.hpp>

void Camera::init(float ratio) {
    _target = glm::vec3(0.0f, 0.0f, 0.0f);  // target world center
    _distance = 2.0f;                       // initial distance
    _azimuth = 0.0f;                        // horizontal rotation angle
    _elevation = 0.0f;                      // vertical rotation angle
    _zoomSpeed = 0.1f;
    _rotateSpeed = 0.005f;
    _perspective = glm::perspective(glm::radians(45.0f), ratio, 0.1f, 100.0f);
    updateTransform();
}

void Camera::updateTransform() {
    float x = _distance * glm::cos(_elevation) * glm::sin(_azimuth);
    float y = _distance * glm::sin(_elevation);
    float z = _distance * glm::cos(_elevation) * glm::cos(_azimuth);
    _position = glm::vec3(x, y, z);
    _transform = glm::lookAt(_position, _target, glm::vec3(0.0f, 1.0f, 0.0f));
}

void Camera::zoom(int zoom) {
    _distance *= 1 - (zoom * _zoomSpeed);
    // avoid zooming reaching zero
    _distance = glm::max(_distance, .001f);
    updateTransform();
}

void Camera::move(glm::vec2 delta) {
    _azimuth -= delta.x * _rotateSpeed;
    _elevation += delta.y * _rotateSpeed;
    // Clamp elevation to avoid flipping the camera
    _elevation = glm::clamp(_elevation, -glm::half_pi<float>(), glm::half_pi<float>());
    updateTransform();
}

void Camera::updateRatio(float ratio) {
    _perspective = glm::perspective(glm::radians(45.0f), ratio, 0.1f, 100.0f);
}
