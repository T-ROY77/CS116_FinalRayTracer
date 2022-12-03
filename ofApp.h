#pragma once

#pragma once

#include "ofMain.h"
#include "ofxGui.h"

#include <glm/gtx/intersect.hpp>

//  General Purpose Ray class 
//
class Ray {
public:
	Ray(glm::vec3 p, glm::vec3 d) { this->p = p; this->d = d; }
	void draw(float t) { ofDrawLine(p, p + t * d); }

	glm::vec3 evalPoint(float t) {
		return (p + t * d);
	}

	glm::vec3 p, d;
};

//  Base class for any renderable object in the scene
//
class SceneObject {
public:
	virtual void draw() = 0;    // pure virtual funcs - must be overloaded
	virtual bool intersect(const Ray& ray, glm::vec3& point, glm::vec3& normal) { cout << "SceneObject::intersect" << endl; return false; }
	virtual glm::vec3 getNormal(const glm::vec3& p) { return glm::vec3(0); }
	virtual glm::vec3 getIntersectionPoint() { return glm::vec3(1); }
	virtual void setImage(ofImage i) {}
	virtual void setImageSpec(ofImage i) {}
	virtual ofColor getDiffuse(glm::vec3 p) { return diffuseColor; }
	virtual ofColor getSpecular(glm::vec3 p) { return specularColor; }
	virtual bool aimPointIntersect(const Ray& ray, glm::vec3& point, glm::vec3& normal) { return false; }

	// any data common to all scene objects goes here
	glm::vec3 position = glm::vec3(0, 0, 0);
	glm::vec3 intersectionPoint;
	float radius = 0;
	float intensity = 0;
	float power = 0;
	float coneAngle = 0;
	float Width = 0;
	float coneAngleDeg = 0;


	// material properties (we will ultimately replace this with a Material class - TBD)
	//
	ofColor diffuseColor = ofColor::grey;    // default colors - can be changed.
	ofColor specularColor = ofColor::lightGray;

	ofImage image;

	bool isSpotLight = false;
	bool isAreaLight = false;

	bool isSelectable = true;
	bool isSelected = false;
	bool hasTexture = false;
	bool hasTextureSpecular = false;
	
};

//  General purpose plane 
//
class Plane : public SceneObject {
public:
	Plane(glm::vec3 p, glm::vec3 n, ofColor diffuse,
		float w, float h) {
		position = p; normal = n;
		width = w;
		height = h;
		diffuseColor = diffuse;
		isSelectable = false;
		if (normal == glm::vec3(0, 1, 0)) plane.rotateDeg(90, 1, 0, 0);
	}
	Plane() {
		normal = glm::vec3(0, 1, 0);
		plane.rotateDeg(90, 1, 0, 0);
		isSelectable = false;

	}
	bool intersect(const Ray& ray, glm::vec3& point, glm::vec3& normal);
	float sdf(const glm::vec3& p);
	glm::vec3 getNormal(const glm::vec3& p) { return this->normal; }
	glm::vec3 getIntersectionPoint() { return this->intersectionPoint; }
	float getWidth() { return width; }
	float getHeight() { return height; }
	ofColor textureMap(glm::vec3 p);
	ofColor specularTextureMap(glm::vec3 p);

	ofColor getDiffuse(glm::vec3 p) {
		if (hasTexture) {
			return textureMap(p);
		}
		else {
			return diffuseColor;
		}
	}

	ofColor getSpecular(glm::vec3 p) {
		if (hasTextureSpecular) {
			return specularTextureMap(p);
		}
		else {
			return specularColor;
		}
	}

	void setImage(ofImage i) {
		image = i;
		hasTexture = true;
	}
	void setImageSpec(ofImage i) {
		imageSpec = i;
		hasTextureSpecular = true;
	}
	void setIntersectionPoint(const glm::vec3& p) { intersectionPoint = p; }
	void draw() {
		plane.setPosition(position);
		plane.setWidth(width);
		plane.setHeight(height);
		plane.setResolution(4, 4);
		plane.draw();
	}


	ofPlanePrimitive plane;
	glm::vec3 normal;
	float width;
	float height;
	glm::vec3 intersectionPoint;
	ofImage image;
	ofImage imageSpec;

	bool hasTexture = false;
	bool hasTextureSpecular = false;

	int floortiles = 1;
	int walltiles = 1;
};




//  General purpose sphere  (assume parametric)
//
class Sphere : public SceneObject {
public:
	Sphere(glm::vec3 p, float r, ofColor diffuse = ofColor::lightGray) { position = p; radius = r; diffuseColor = diffuse; }
	Sphere() {}
	bool intersect(const Ray& ray, glm::vec3& point, glm::vec3& normal) {
		bool intersect = (glm::intersectRaySphere(ray.p, glm::normalize(ray.d), position, radius, point, normal));
		setNormal(normal);
		intersectionPoint = point;
		return intersect;
	}
	void draw() {
		if (isSelected) {
			ofNoFill();
		}
		else {
			ofFill();
		}
		ofDrawSphere(position, radius);
	}




	void setNormal(const glm::vec3& p) { normal = p; }

	glm::vec3 getNormal(const glm::vec3& p) { return glm::normalize(normal); }

	ofColor getDiffuse(glm::vec3 p) { return diffuseColor; }

	glm::vec3 normal;

};


class Light : public SceneObject {
public:
	Light(glm::vec3 p, glm::vec3 aimPos, float i, float angle, float width) { 
		position = p; 
		intensity = i; 
		power = 100;
		coneAngleDeg = angle;
		coneAngle = tan(glm::radians(angle)) * coneHeight;
		Width = width;
		radius = .5;
		aimPoint = aimPos;
		planeHeight = width;
		setPointLight();
	}
	Light() {}

	void setPointLight() {
		isSpotLight = false;
		isAreaLight = false;
	}

	void setSpotLight() {
		isSpotLight = true;
		isAreaLight = false;
	}

	void setAreaLight() {
		isSpotLight = false;
		isAreaLight = true;
	}


	bool intersect(const Ray& ray, glm::vec3& point, glm::vec3& normal) {
		if (isAreaLight) {
			Plane p = Plane(position, glm::normalize(position - aimPoint), ofColor::grey, planeHeight, planeHeight);
			return p.intersect(ray, point, normal);
		}
		else {
			return (glm::intersectRaySphere(ray.p, ray.d, position, radius, point, normal));
		}
	}

	bool aimPointIntersect(const Ray& ray, glm::vec3& point, glm::vec3& normal) {
		if (isSpotLight || isAreaLight) {
			return (glm::intersectRaySphere(ray.p, ray.d, aimPoint, aimPointRadius, point, normal));
		}
		else {
			return false;
		}
	}

	void draw() {
		ofSetColor(ofColor::gray);
		if (isSelected) {
			ofNoFill();
		}
		else {
			ofFill();
		}

		if (isSpotLight) {
			// draw a cone object oriented towards aim position using the lookAt transformation
	// matrix.  The "up" vector is (0, 1, 0)
	//
			ofPushMatrix();
			glm::mat4 m = glm::lookAt(position, aimPoint, glm::vec3(0, 1, 0));
			ofMultMatrix(glm::inverse(m));
			ofRotate(-90, 1, 0, 0);
			ofDrawCone(coneAngle, 5);
			ofPopMatrix();
			ofDrawLine(position, aimPoint);

		}
		else if (isAreaLight) {
			//draw rectangle
			ofPushMatrix();
			glm::mat4 m = glm::lookAt(position, aimPoint, glm::vec3(0, 1, 0));
			ofMultMatrix(glm::inverse(m));
			ofDrawRectangle(glm::vec3(-Width / 2, -Width / 2, 0), Width, Width);
			ofPopMatrix();
			ofDrawLine(position, aimPoint);
		}
		else {
			ofDrawSphere(position, radius);
		}
	}

	void setIntensity(float i) {
		intensity = i;
	}

	glm::vec3 direction = glm::vec3(0);
	glm::vec3 aimPoint = glm::vec3(0);


	float aimPointRadius = .2;
	float length = 10;
	float coneHeight = 5;
	int planeHeight;
	
};

// view plane for render camera
// 
class  ViewPlane : public Plane {
public:
	ViewPlane(glm::vec2 p0, glm::vec2 p1) { min = p0; max = p1; }

	ViewPlane() {                         // create reasonable defaults (6x4 aspect)
		min = glm::vec2(-3, -2);
		max = glm::vec2(3, 2);
		position = glm::vec3(0, 0, 5);
		normal = glm::vec3(0, 0, 1);      // viewplane currently limited to Z axis orientation
	}

	void setSize(glm::vec2 min, glm::vec2 max) { this->min = min; this->max = max; }
	float getAspect() { return width() / height(); }

	glm::vec3 toWorld(float u, float v);   //   (u, v) --> (x, y, z) [ world space ]

	void draw() {
		ofDrawRectangle(glm::vec3(min.x, min.y, position.z), width(), height());
	}

	float width() {
		return (max.x - min.x);
	}
	float height() {
		return (max.y - min.y);
	}

	// some convenience methods for returning the corners
	//
	glm::vec2 topLeft() { return glm::vec2(min.x, max.y); }
	glm::vec2 topRight() { return max; }
	glm::vec2 bottomLeft() { return min; }
	glm::vec2 bottomRight() { return glm::vec2(max.x, min.y); }

	//  To define an infinite plane, we just need a point and normal.
	//  The ViewPlane is a finite plane so we need to define the boundaries.
	//  We will define this in terms of min, max  in 2D.  
	//  (in local 2D space of the plane)
	//  ultimately, will want to locate the ViewPlane with RenderCam anywhere
	//  in the scene, so it is easier to define the View rectangle in a local'
	//  coordinate system.
	//
	glm::vec2 min, max;
};


//  render camera  - currently must be z axis aligned (we will improve this in project 4)
//
class RenderCam : public SceneObject {
public:
	RenderCam() {
		position = glm::vec3(0, 0, 10);
		aim = glm::vec3(0, 0, -1);
	}
	Ray getRay(float u, float v);
	void draw() { ofDrawBox(position, 1.0); };
	void drawFrustum();

	glm::vec3 aim;
	ViewPlane view;          // The camera viewplane, this is the view that we will render 
};


class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void mouseEntered(int x, int y);
		void mouseExited(int x, int y);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);

		void createSphere();
		void deleteSphere();
		void createLight();
		void deleteLight();
		void rayTrace();
		void drawGrid();
		void drawAxis(glm::vec3 position);
		bool mouseToDragPlane(int x, int y, glm::vec3& point);
		bool objSelected() { return (selected.size() ? true : false); };
		ofColor ambient(ofColor diffuse);
		ofColor lambert(const glm::vec3& p, const glm::vec3& norm, const ofColor diffuse, float distance, Ray r, Light light);
		ofColor phong(const glm::vec3& p, const glm::vec3& norm, const ofColor diffuse, const ofColor specular, float power, float distance, Ray r, Light light);
		ofColor shade(const glm::vec3& p, const glm::vec3& norm, const ofColor diffuse, float distance, const ofColor specular, float power, Ray r);
		ofColor spotLightPhong(const glm::vec3& p, const glm::vec3& norm, const ofColor diffuse, const ofColor specular, float power, float distance, Ray r, Light light);
		ofColor spotLightLambert(const glm::vec3& p, const glm::vec3& norm, const ofColor diffuse, float distance, Ray r, Light light);
		ofColor areaLightPhong(const glm::vec3& p, const glm::vec3& norm, const ofColor diffuse, const ofColor specular, float power, float distance, Ray r, Light light);
		ofColor areaLightLambert(const glm::vec3& p, const glm::vec3& norm, const ofColor diffuse, float distance, Ray r, Light light);



		const float zero = 0.0;

		bool bHide = true;
		bool bShowImage = false;

		ofEasyCam  mainCam;
		ofCamera sideCam;
		ofCamera previewCam;
		ofCamera* theCam;    // set to current camera either mainCam or sideCam

		// set up one render camera to render image through
		//
		RenderCam renderCam;
		ofImage image;

		//texture images
		//
		ofImage groundTexture;
		ofImage groundTextureSpecular;

		ofImage wallTexture;
		ofImage wallTextureSpecular;


		//object vectors
		//
		vector<SceneObject*> scene;
		vector<Light*> light;
		vector<Sphere*> aimPoint;

		vector<SceneObject*> selected;

		int imageWidth = 1200;
		int imageHeight = 800;
		int closestIndex = 0;
		float sphereRadius = .5;
		float aimPointRadius = .5;
		glm::vec3 lastPoint;
		int numofLights = 0;

		//state variables
		//
		bool drawImage = false;
		bool trace = false;
		bool background = true;
		bool blocked = false;
		bool texture = false;
		bool bDrag = false;

		//GUI
		//
		ofxFloatSlider power;
		ofxFloatSlider intensity;
		ofxFloatSlider scale;
		ofxFloatSlider spotLightAngle;
		ofxFloatSlider areaLightWidth;
		ofxIntSlider lightTypeToggle;
		ofxVec3Slider color;
		ofxLabel lightLabel;
		ofxLabel sphereLabel;
		ofxPanel gui;
};
