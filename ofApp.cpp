#include "ofApp.h"

#include <glm/gtx/intersect.hpp>

//  (c) Troy Perez - 17 October 2022


// Intersect Ray with Plane  (wrapper on glm::intersect*
//

bool Plane::intersect(const Ray& ray, glm::vec3& point, glm::vec3& normalAtIntersect) {
	float dist;
	bool insidePlane = false;
	bool hit = glm::intersectRayPlane(ray.p, ray.d, position, this->normal, dist);
	if (hit) {
		Ray r = ray;
		point = r.evalPoint(dist);
		setIntersectionPoint(r.evalPoint(dist));

		normalAtIntersect = this->normal;
		glm::vec2 xrange = glm::vec2(position.x - width / 2, position.x + width
			/ 2);
		glm::vec2 zrange = glm::vec2(position.z - height / 2, position.z +
			height / 2);
		if (point.x < xrange[1] && point.x > xrange[0] && point.z < zrange[1]
			&& point.z > zrange[0]) {
			insidePlane = true;
		}
	}
	return insidePlane;
}

// Convert (u, v) to (x, y, z) 
// We assume u,v is in [0, 1]
//
glm::vec3 ViewPlane::toWorld(float u, float v) {
	float w = width();
	float h = height();
	return (glm::vec3((u * w) + min.x, (v * h) + min.y, position.z));
}

// Get a ray from the current camera position to the (u, v) position on
// the ViewPlane
//
Ray RenderCam::getRay(float u, float v) {
	glm::vec3 pointOnPlane = view.toWorld(u, v);
	return(Ray(position, glm::normalize(pointOnPlane - position)));
}


//--------------------------------------------------------------
void ofApp::setup() {
	image.allocate(imageWidth, imageHeight, ofImageType::OF_IMAGE_COLOR);

	gui.setup();
	gui.add(intensity.setup("Light intensity", .2, .05, 2));
	gui.add(areaLightIntensity.setup("area light intensity", .5, .05, 2));
	gui.add(power.setup("Phong p", 100, 10, 10000));
	bHide = true;

	theCam = &mainCam;
	mainCam.setPosition(glm::vec3(0, 75, 75));
	mainCam.setTarget(glm::vec3(0, 0, 0));
	imageCam.setPosition(glm::vec3(0, 500, 500));
	imageCam.lookAt(glm::vec3(0, 0, 0));
	previewCam.setPosition(renderCam.position);
	previewCam.lookAt(glm::vec3(0, 0, -1));

	cout << "h to toggle GUI" << endl;
	cout << "c to toggle camera mode" << endl;
	cout << "t to start ray tracer" << endl;
	cout << "d to show render" << endl;
	cout << "F1 for easy cam" << endl;
	cout << "F2 for render cam preview" << endl;
	cout << "arrow keys to change selected cone angle" << endl;

	aimPoint.clear();
	areaLightPos.clear();
	angle.clear();

	aimPoint.push_back(glm::vec3(1, -5, 0));
	areaLightPos.push_back(glm::vec3(-20, 30, 45));
	angle.push_back(15);


	//aimPoint.push_back(glm::vec3(3, 3, 0));
	areaLightPos.push_back(glm::vec3(-50, 30, 45));
	angle.push_back(15);
}

//updates the angle of the cone width
//
void ofApp::updateAngle(bool increase) {
	if (increase) {
		if (!mainCam.getMouseInputEnabled()) {
			if (angle[lightIndex] < 25) angle[lightIndex] += .5;
			areaLights[lightIndex]->Height += .5;
		}
	}
	else {
		if (!mainCam.getMouseInputEnabled()) {
			if (angle[lightIndex] > 5) angle[lightIndex] -= .5;
			areaLights[lightIndex]->Height -= .5;

		}
	}
}

//--------------------------------------------------------------
void ofApp::update() {

}

//--------------------------------------------------------------
void ofApp::rayTrace() {

	cout << "drawing..." << endl;

	ofColor closest;
	float distance = FLT_MIN;
	float close = FLT_MAX;
	closestIndex = 0;
	for (int i = 0; i < image.getWidth(); i++) {
		for (int j = 0; j < image.getHeight(); j++) {
			background = true;																//reset variables every pixel
			distance = FLT_MIN;
			close = FLT_MAX;
			closestIndex = 0;

			float u = (i + .5) / image.getWidth();
			float v = 1 - (j + .5) / image.getHeight();

			Ray r = renderCam.getRay(u, v);
			for (int k = 0; k < scene.size(); k++) {
				if (scene[k]->intersect(r, scene[k]->intersectionPoint, glm::vec3(0, 1, 0))) {
					background = false;														//if intersected with scene object, pixel is not background

					distance = glm::distance(r.p, scene[k]->position);						//calculate distance of intersection
					if (distance < close)													//if current object is closest to viewplane
					{
						closestIndex = k;													//save index of closest object
						close = distance;													//set threshold to new closest distance
					}
				}
			}
			if (!background) {
				//add shading contribution
				closest = shade(r.evalPoint(close), scene[closestIndex]->getNormal(glm::vec3(0, 0, 0)), scene[closestIndex]->diffuseColor, close, ofColor::lightGray, power, r);
				image.setColor(i, j, closest);
			}
			else if (background) {
				image.setColor(i, j, ofColor::black);
			}
		}
	}

	image.save("output.png");
	image.load("output.png");

	cout << "render saved" << endl;
}


//--------------------------------------------------------------
//calculates lambert shading for point lights
//returns shaded color
ofColor ofApp::lambert(const glm::vec3& p, const glm::vec3& norm, const ofColor diffuse, float distance, Ray r, Light light) {
	ofColor lambert = ofColor(0, 0, 0);

	float distance1 = glm::distance(light.position, p);


	glm::vec3 l = glm::normalize(light.position - p);
	lambert += diffuse * (light.intensity / distance1 * distance1) * (glm::max(zero, glm::dot(norm, l)));
	return lambert;
}


//--------------------------------------------------------------
//calculates point light shading including:
// lambert
// phong
//returns shaded color
ofColor ofApp::phong(const glm::vec3& p, const glm::vec3& norm, const ofColor diffuse, const ofColor specular, float power, float distance, Ray r, Light light) {
	ofColor phong = ofColor(0, 0, 0);
	glm::vec3 h = glm::vec3(0);
	float distance1 = glm::distance(light.position, p);

	glm::vec3 l = glm::normalize(light.position - p);
	glm::vec3 v = glm::normalize(renderCam.position - p);
	h = glm::normalize(l + v);



	phong += (lambert(p, norm, diffuse, distance1, r, light)) + (specular * (light.intensity / distance1 * distance1) * glm::pow(glm::max(zero, glm::dot(norm, h)), power));
	return phong;
}

//--------------------------------------------------------------
//calculates lambert shading from spot lights
//returns shaded color
ofColor ofApp::areaLightLambert(const glm::vec3& p, const glm::vec3& norm, const ofColor diffuse, float distance, Ray r, areaLight light) {
	ofColor lambert = ofColor(0, 0, 0);
	glm::vec3 point1, point2, point3, point4, normal;

	float distance1 = glm::distance(light.position, p);


	//Ray s = Ray(renderCam.position, glm::normalize(p - renderCam.position));



	//change algorithm here
	//
	//
	// project ray from point to rectangle plane
	// if intersection is right angle, shade
	// 
	// 
	//Ray s = Ray(light.topRightCorner(), glm::normalize(p - light.topRightCorner()));

	//ofDrawLine(topLeftCorner(), glm::vec3(aimPoint.x - Height, aimPoint.y + Height, aimPoint.z));
	//ofDrawLine(bottomRightCorner(), glm::vec3(aimPoint.x + Height, aimPoint.y - Height, aimPoint.z));
	//ofDrawLine(bottomLeftCorner(), glm::vec3(aimPoint.x - Height, aimPoint.y - Height, aimPoint.z));



	Ray tr = Ray(light.topRightCorner(), glm::vec3(light.aimPoint.x + light.Height, light.aimPoint.y + light.Height, light.aimPoint.z) - light.topRightCorner());
	Ray tl = Ray(light.topLeftCorner(), glm::vec3(light.aimPoint.x - light.Height, light.aimPoint.y + light.Height, light.aimPoint.z) - light.topLeftCorner());
	Ray br = Ray(light.bottomRightCorner(), glm::vec3(light.aimPoint.x + light.Height, light.aimPoint.y - light.Height, light.aimPoint.z) - light.bottomRightCorner());
	Ray bl = Ray(light.bottomLeftCorner(), glm::vec3(light.aimPoint.x - light.Height, light.aimPoint.y - light.Height, light.aimPoint.z) - light.bottomLeftCorner());

	scene[0]->intersect(tr, point1, normal);
	scene[0]->intersect(tl, point2, normal);
	scene[0]->intersect(br, point3, normal);
	scene[0]->intersect(bl, point4, normal);


	//test if p is inside 4 point box
	float width = glm::distance(point1, point2);
	float recArea = width * width;
	//float recArea = light.Height * light.Height;


	//triangle one
	float a = glm::distance(point1, point2);
	float b = glm::distance(p, point2);
	float c = glm::distance(point1, p);
	float s = (a + b + c)/2;
	float triArea1 = glm::sqrt(s* (s - a) * (s - b) * (s - c));

	//triangle two
	a = glm::distance(point2, point3);
	b = glm::distance(p, point2);
	c = glm::distance(point3, p);
	s = (a + b + c)/2;
	float triArea2 = glm::sqrt(s * (s - a) * (s - b) * (s - c));

	//triangle three
	a = glm::distance(point3, point4);
	b = glm::distance(p, point3);
	c = glm::distance(point4, p);
	s = (a + b + c)/2;
	float triArea3 = glm::sqrt(s * (s - a) * (s - b) * (s - c));

	//triangle four
	a = glm::distance(point1, point4);
	b = glm::distance(p, point1);
	c = glm::distance(point4, p);
	s = (a + b + c)/2;
	float triArea4 = glm::sqrt(s * (s - a) * (s - b) * (s - c));



	float triAreaSum = triArea1 + triArea2 + triArea3 + triArea4;

	//float dis = glm::distance(light.aimPoint, p);

	





	/*
	if (triAreaSum <= recArea) {		//if p is inside spot light illumination area
		glm::vec3 l = glm::normalize(light.position - p);
		lambert += diffuse * (light.intensity / distance1 * distance1) * (glm::max(zero, glm::dot(norm, l)));
	}
	*/

	if ((0<glm::dot(p - point1, point2 - point1) < glm::dot(point2 - point1, point2 - point1)) || ( 0< glm::dot(p - point1, point3 - point1) < glm::dot(point3 - point1, point3 - point1 ))) {		//if p is inside spot light illumination area
		glm::vec3 l = glm::normalize(light.position - p);
		lambert += diffuse * (light.intensity / distance1 * distance1) * (glm::max(zero, glm::dot(norm, l)));
	}

	return lambert;
}


//--------------------------------------------------------------
//adds shading contribution
//calculates shadows
//returns shaded color
ofColor ofApp::shade(const glm::vec3& p, const glm::vec3& norm, const ofColor diffuse, float distance, const ofColor specular, float power, Ray r) {
	ofColor shaded = (0, 0, 0);
	glm::vec3 p1 = p;

	//loop through all lights
	for (int i = 0; i < light.size(); i++) {
		blocked = false;

		//test for shadows
		if (closestIndex < 2) {								//if the closest object is one of the planes
			if (scene[0]->intersect(r, p1, glm::vec3(0, 1, 0))) {													//check if current point intersected with ground plane

				Ray shadowRay = Ray(scene[0]->getIntersectionPoint(), light[i]->position - scene[0]->getIntersectionPoint());

				//check all sphere objects
				for (int j = 2; j < scene.size(); j++) {
					if (scene[j]->intersect(shadowRay, p1, scene[j]->getNormal(p1))) {
						blocked = true;
					}
				}
			}
		}
		if (!blocked) {
			//point light shading
			shaded += phong(p, norm, diffuse, specular, power, distance, r, *light[i]);
		}
	}

	for (int i = 0; i < areaLights.size(); i++) {
		blocked = false;

		//test for shadows
		if (scene[0]->intersect(r, p1, glm::vec3(0, 1, 0))) {													//check if current point intersected with ground plane
			Ray shadowRay = Ray(scene[0]->getIntersectionPoint(), areaLights[i]->position - scene[0]->getIntersectionPoint());

			//check all sphere objects
			for (int j = 2; j < scene.size(); j++) {
				if (scene[j]->intersect(shadowRay, p1, scene[j]->getNormal(p1))) {
					blocked = true;
				}
			}
		}
		if (!blocked) {
			//area light shading
			shaded += areaLightLambert(p, norm, diffuse, distance, r, *areaLights[i]);

		}
	}

	return shaded;
}
//--------------------------------------------------------------
void ofApp::draw() {

	ofSetDepthTest(true);

	theCam->begin();

	scene.clear();

	scene.push_back(new Plane(glm::vec3(0, -5, 0), glm::vec3(0, 1, 0), ofColor::tan, 600, 400));				//ground plane

	scene.push_back(new Plane(glm::vec3(0, 0, -10), glm::vec3(0, 0, 1), ofColor::darkGrey, 600, 400));				//wall plane

	scene.push_back(new Sphere(glm::vec3(0, 1, -2), 1, ofColor::purple));											//purple sphere

	scene.push_back(new Sphere(glm::vec3(-1, 0, 1), 1, ofColor::blue));												//blue sphere

	scene.push_back(new Sphere(glm::vec3(.5, 0, 0), 1, ofColor::lightPink));											//pink sphere


	light.clear();

	//light.push_back(new Light(glm::vec3(100, 150, 150), .2));			//top right light

	areaLights.clear();

	for (int i = 0; i < aimPoint.size(); i++) {
		areaLights.push_back(new areaLight(areaLightPos[i], aimPoint[i], 2, angle[i]));
	}

	//draw all scene objects
	for (int i = 0; i < scene.size(); i++) {
		ofColor color = scene[i]->diffuseColor;
		ofSetColor(color);
		scene[i]->draw();
	}


	//draw all lights
	for (int i = 0; i < light.size(); i++) {
		light[i]->setIntensity(intensity);
		light[i]->draw();
	}

	//draw all spotlights
	for (int i = 0; i < areaLights.size(); i++) {
		areaLights[i]->setIntensity(areaLightIntensity);
		areaLights[i]->draw();
	}

	ofSetColor(ofColor::red);
	ofFill();
	renderCam.view.draw();



	theCam->end();


	if (!bHide) {
		ofSetDepthTest(false);
		gui.draw();
	}

	//draw render
	if (drawImage) {
		ofSetColor(ofColor::white);
		image.draw(0, 0);
	}


}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {
	switch (key) {
	case OF_KEY_F1:
		theCam = &mainCam;
		break;
	case OF_KEY_F2:
		theCam = &previewCam;
		break;
	case 'd':
		drawImage = !drawImage;
		if (theCam == &imageCam)  theCam = &mainCam;
		else  theCam = &imageCam;
		break;
	case 't':
		rayTrace();
		break;
	case 'h':
		bHide = !bHide;
		break;
	case OF_KEY_UP:
		if (lightSelect) updateAngle(true);
		break;
	case OF_KEY_DOWN:
		if (lightSelect) updateAngle(false);
		break;
	case 'c':
		if (mainCam.getMouseInputEnabled()) mainCam.disableMouseInput();
		else mainCam.enableMouseInput();
		break;
	default:
		break;
	}
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key) {

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y) {

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button) {
	if (!mainCam.getMouseInputEnabled()) {

		//moving aim point
		//
		if (aimPointDrag) {
			glm::vec3 screen3dpt = theCam->screenToWorld(glm::vec3(x, y, 0));
			glm::vec3 rayOrigin = theCam->getPosition();
			glm::vec3 rayDir = glm::normalize(screen3dpt - rayOrigin);
			r = Ray(rayOrigin, rayDir);
			p.intersect(r, glm::vec3(0), glm::vec3(0));							//move along a plane parellel to the camera view

			aimPoint[lightIndex] = p.getIntersectionPoint();

		}

		//moving spot light position
		//
		else if (lightDrag) {
			glm::vec3 screen3dpt = theCam->screenToWorld(glm::vec3(x, y, 0));
			glm::vec3 rayOrigin = theCam->getPosition();
			glm::vec3 rayDir = glm::normalize(screen3dpt - rayOrigin);
			r = Ray(rayOrigin, rayDir);
			p.intersect(r, glm::vec3(0), glm::vec3(0));						//move along a plane parellel to the camera view

			areaLightPos[lightIndex] = p.getIntersectionPoint();
		}
	}
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button) {
	glm::vec3 screen3dpt = theCam->screenToWorld(glm::vec3(x, y, 0));
	glm::vec3 rayOrigin = theCam->getPosition();
	glm::vec3 rayDir = glm::normalize(screen3dpt - rayOrigin);
	aimPointSelect = false;
	lightSelect = false;


	r = Ray(rayOrigin, rayDir);
	glm::vec3 point, normal;
	for (int i = 0; i < areaLights.size(); i++) {

		//check if mouse intersected with aimpoint
		aimPointSelect = glm::intersectRaySphere(r.p, r.d, areaLights[i]->aimPoint, areaLights[i]->Height / 10, point, normal);
		if (aimPointSelect) {
			planeNormal = glm::normalize(mainCam.getPosition() - screen3dpt);
			p = Plane(aimPoint[i], planeNormal, ofColor::grey, 20, 20);
			lightIndex = i;
			aimPointDrag = true;
		}

		//check if mouse intersected with spot light
		lightSelect = glm::intersectRaySphere(r.p, r.d, areaLights[i]->position, areaLights[i]->Height / 5, point, normal);
		if (lightSelect && !aimPointSelect) {
			planeNormal = glm::normalize(mainCam.getPosition() - areaLightPos[i]);
			p = Plane(areaLightPos[i], planeNormal, ofColor::grey, 20, 20);
			lightIndex = i;
			lightDrag = true;
		}
	}
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button) {
	lightDrag = false;
	aimPointDrag = false;
	aimPointSelect = false;
	lightSelect = false;
}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y) {

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y) {

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h) {

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg) {

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo) {

}