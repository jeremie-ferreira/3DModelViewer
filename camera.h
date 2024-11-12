#pragma once
#include <glm/glm.hpp>
#include "event.h"

/**
 * The Camera class manages the view and projection transformations for rendering.
 * It allows zooming, movement, and ratio adjustments.
 */
class Camera {
public:
    /**
     * Initializes the camera with a specified aspect ratio.
     * @param ratio The aspect ratio of the screen (width / height).
     */
    void init(float ratio);

    /**
     * Adjusts the zoom level of the camera.
     * @param zoom The zoom amount to apply; positive to zoom in, negative to zoom out.
     */
    void zoom(int zoom);

    /**
     * Moves the camera based on a delta change in position.
     * @param delta A 2D vector representing the change in position.
     */
    void move(glm::vec2 delta);

    /**
     * Updates the camera's aspect ratio when the screen size changes.
     * @param ratio The new aspect ratio.
     */
    void updateRatio(float ratio);

    /**
     * Retrieves the camera's transformation matrix.
     * @return The 4x4 transformation matrix representing the camera's view.
     */
    glm::mat4 getTransform() const { return _transform; }

    /**
     * Retrieves the camera's perspective projection matrix.
     * @return The 4x4 perspective matrix for rendering 3D depth.
     */
    glm::mat4 getPerspective() const { return _perspective; }

    /**
     * Retrieves the camera's current position in 3D space.
     * @return The 3D position vector of the camera.
     */
    glm::vec3 getPosition() const { return _position; }

private:
    /**
     * Updates the camera's transformation matrix based on its position, target, and orientation.
     */
    void updateTransform();

    glm::mat4 _transform;     // The view transformation matrix of the camera
    glm::mat4 _perspective;   // The perspective projection matrix for 3D rendering
    glm::vec3 _position;      // Position of the camera in 3D space
    glm::vec3 _target;        // Target point that the camera is looking at
    float _distance;          // Distance from the target (used for zoom)
    float _azimuth;           // Azimuth angle for horizontal rotation (left-right)
    float _elevation;         // Elevation angle for vertical rotation (up-down)
    float _zoomSpeed;         // Speed at which the camera zooms in/out
    float _rotateSpeed;       // Speed at which the camera rotates around the target
};
