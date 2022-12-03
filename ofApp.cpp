#include "ofApp.h"
#include <glm/gtx/intersect.hpp>


//implement area light lambert

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
//setup gui, scene objects, lights, textures, and camera
//
void ofApp::setup(){
	image.allocate(imageWidth, imageHeight, ofImageType::OF_IMAGE_COLOR);

	gui.setup();
	string lights = "Light Parameters";
	//gui.add(lightLabel.setup(lights));

	gui.add(intensity.setup("Light intensity", .2, .05, .5));
	gui.add(power.setup("Phong p", 100, 10, 10000));
	gui.add(spotLightAngle.setup("Spot Light Angle", 5, 5, 30));
	gui.add(areaLightWidth.setup("Area Light Width", 5, 1, 10));
	gui.add(lightTypeToggle.setup("Light Type", 1, 1, 3));





	string spheres = "Sphere Parameters";
	//gui.add(sphereLabel.setup(spheres));

	gui.add(scale.setup("sphere scale", .5, .1, 3));
	gui.add(color.setup("sphere color", glm::vec3(0,0,255), glm::vec3(0,0,0), glm::vec3(255,255,255)));


	bHide = false;

	theCam = &mainCam;
	mainCam.setDistance(10);
	mainCam.setNearClip(.1);
	sideCam.setPosition(glm::vec3(5, 0, 0));
	sideCam.lookAt(glm::vec3(0, 0, 0));
	sideCam.setNearClip(.1);
	previewCam.setFov(90);
	previewCam.setPosition(0, 0, 10);
	previewCam.lookAt(glm::vec3(0, 0, -1));


	groundTexture.load("bamboo.jpg");
	groundTextureSpecular.load("bamboo_spec.jpg");

	wallTexture.load("ceramic_wall.jpg");
	wallTextureSpecular.load("ceramic_wall_spec.jpg");

	scene.clear();

	scene.push_back(new Plane(glm::vec3(-1, -2, 0), glm::vec3(0, 1, 0), ofColor::darkBlue, 12, 10));				//ground plane

	scene.push_back(new Plane(glm::vec3(-1, 1, -5), glm::vec3(0, 0, 1), ofColor::darkGray, 20, 10));	        	//wall plane
	
	aimPoint.clear();

	aimPoint.push_back(new Sphere(glm::vec3(1, -2, 0), aimPointRadius));

	light.clear();

	light.push_back(new Light(glm::vec3(10, 5, 5), aimPoint[0]->position, .2, 15, 5));			//top right light
	numofLights++;

	scene[0]->setImage(groundTexture);
	scene[0]->setImageSpec(groundTextureSpecular);


	scene[1]->setImage(wallTexture);
	scene[1]->setImageSpec(wallTextureSpecular);

	cout << "t to start ray tracer" << endl;
	cout << "r to toggle render image" << endl;
	cout << "c to toggle camera control" << endl;
	cout << "j to create new sphere" << endl;
	cout << "d to delete selected sphere" << endl;
	cout << "l to create new light" << endl;
	cout << "k to delete selected light" << endl;
	cout << "h to toggle gui" << endl;
	cout << "select a sphere or a light to change the parameters" << endl;
}

//--------------------------------------------------------------
//update scene object and light parameters based on gui
//
void ofApp::update() {

	float angle = spotLightAngle;

	glm::vec3 colorSlider = color;

	//update scene object parameters
	//
	for (int i = 0; i < scene.size(); i++) {
		if (objSelected()) {
			if (scene[i] == selected[0]) {
				scene[i]->radius = scale;
				scene[i]->diffuseColor = ofColor(colorSlider.x, colorSlider.y, colorSlider.z);
			}
		}
	}
	

	//update light parameters
	//
	for (int i = 0; i < light.size(); i++) {
		light[i]->aimPoint = aimPoint[i]->position;
		if (objSelected()) {
			if (light[i] == selected[0]) {
				light[i]->radius = scale;
				light[i]->intensity = intensity;
				light[i]->power = power;
				light[i]->coneAngleDeg = angle;
				light[i]->coneAngle = tan(glm::radians(angle)) * light[i]->coneHeight;
				light[i]->Width = areaLightWidth;
				if (lightTypeToggle == 1) { light[i]->setPointLight(); }
				else if (lightTypeToggle == 2) { light[i]->setSpotLight(); }
				else if (lightTypeToggle = 3){ light[i]->setAreaLight(); }
			}
		}
	}


}

//--------------------------------------------------------------
//draw all scene objects and lights
void ofApp::draw(){

	ofSetDepthTest(true);

	theCam->begin();

	//draw all scene objects
	for (int i = 0; i < scene.size(); i++) {
		ofColor color = scene[i]->diffuseColor;
		ofSetColor(color);
		scene[i]->draw();
	}

	//draw all lights
	for (int i = 0; i < light.size(); i++) {
		light[i]->draw();
	}

	//draw all aim points
	for (int i = 0; i < aimPoint.size(); i++) {
		if (light[i]->isAreaLight || light[i]->isSpotLight) {
			aimPoint[i]->draw();
		}
	}

	//renderCam.view.draw();


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
void ofApp::keyPressed(int key){
	switch (key) {
	case OF_KEY_F1:
		theCam = &mainCam;
		break;
	case OF_KEY_F2:
		theCam = &sideCam;
		break;
	case OF_KEY_F3:
		theCam = &previewCam;
		break;
	case 'r':
		drawImage = !drawImage;
		break;
	case 't':
		rayTrace();
		break;
	case 'h':
		bHide = !bHide;
		break;
	case 'c':
		if (mainCam.getMouseInputEnabled()) mainCam.disableMouseInput();
		else mainCam.enableMouseInput();
		break;
	case 'j':
		createSphere();
		break;
	case 'd':
		deleteSphere();
		break;
	case 'l':
		createLight();
		break;
	case 'k':
		deleteLight();
		break;
	default:
		break;
	}
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
//allow for moving of scene objects and lights
//
void ofApp::mouseDragged(int x, int y, int button){
	if (objSelected() && bDrag) {
		glm::vec3 point;
		mouseToDragPlane(x, y, point);
		selected[0]->position += (point - lastPoint);
		lastPoint = point;
	}

}

//--------------------------------------------------------------
//test for mouse intersection
//
void ofApp::mousePressed(int x, int y, int button){
	// if moving camera, don't allow mouse interaction
	//
	if (mainCam.getMouseInputEnabled()) return;

	// clear selection list
	//
	selected.clear();

	//reset all isSelected variables
	//
	for (int i = 0; i < scene.size(); i++) {
		scene[i]->isSelected = false;
	}

	for (int i = 0; i < light.size(); i++) {
		light[i]->isSelected = false;
	}

	for (int i = 0; i < aimPoint.size(); i++) {
		aimPoint[i]->isSelected = false;
	}


	//
	// test if something selected
	//
	vector<SceneObject*> hits;

	glm::vec3 p = theCam->screenToWorld(glm::vec3(x, y, 0));
	glm::vec3 d = p - theCam->getPosition();
	glm::vec3 dn = glm::normalize(d);

	// check for selection of scene objects
	//
	for (int i = 0; i < scene.size(); i++) {

		glm::vec3 point, norm;

		//  We hit an object
		//
		if (scene[i]->isSelectable && scene[i]->intersect(Ray(p, dn), point, norm)) {
			hits.push_back(scene[i]);
		}
	}

	// check for selection of light objects
	//
	for (int i = 0; i < light.size(); i++) {

		glm::vec3 point, norm;

		//  We hit an object
		//
		if (light[i]->isSelectable && light[i]->intersect(Ray(p, dn), point, norm)) {
			hits.push_back(light[i]);
		}
	}

	
	// check for selection of light aim points
	//
	for (int i = 0; i < aimPoint.size(); i++) {

		glm::vec3 point, norm;

		//  We hit an object
		//
		if (aimPoint[i]->intersect(Ray(p, dn), point, norm)) {
			aimPoint[i]->isSelected = true;
			hits.push_back(aimPoint[i]);
		}
	}
	


	// if we selected more than one, pick nearest
	//
	SceneObject* selectedObj = NULL;
	if (hits.size() > 0) {
		selectedObj = hits[0];
		float nearestDist = std::numeric_limits<float>::infinity();
		for (int n = 0; n < hits.size(); n++) {
			float dist = glm::length(hits[n]->position - theCam->getPosition());
			if (dist < nearestDist) {
				nearestDist = dist;
				selectedObj = hits[n];
			}
		}
	}

	//handle object selection
	//
	if (selectedObj) {
		selected.push_back(selectedObj);
		selectedObj->isSelected = true;


		//reset gui variables to selected object parameters
		//
		float r = selectedObj->diffuseColor.r;
		float g = selectedObj->diffuseColor.g;
		float b = selectedObj->diffuseColor.b;
		color = glm::vec3(r,g,b);
		scale = selectedObj->radius;
		intensity = selectedObj->intensity;
		power = selectedObj->power;
		spotLightAngle = selectedObj->coneAngleDeg;
		areaLightWidth = selectedObj->Width;

		if (selectedObj->isSpotLight) {
			lightTypeToggle = 2;
		}
		else if (selectedObj->isAreaLight) {
			lightTypeToggle = 3;
		}
		else {
			lightTypeToggle = 1;
		}

		bDrag = true;
		mouseToDragPlane(x, y, lastPoint);
	}
	else {
		selected.clear();
	}
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
	bDrag = false;
}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}

//--------------------------------------------------------------
//  This projects the mouse point in screen space (x, y) to a 3D point on a plane
//  normal to the view axis of the camera passing through the point of the selected object.
//  If no object selected, the plane passing through the world origin is used.
//
bool ofApp::mouseToDragPlane(int x, int y, glm::vec3& point) {
	glm::vec3 p = theCam->screenToWorld(glm::vec3(x, y, 0));
	glm::vec3 d = p - theCam->getPosition();
	glm::vec3 dn = glm::normalize(d);

	float dist;
	glm::vec3 pos;
	if (objSelected()) {
		pos = selected[0]->position;
	}
	else pos = glm::vec3(0, 0, 0);
	if (glm::intersectRayPlane(p, dn, pos, glm::normalize(theCam->getZAxis()), dist)) {
		point = p + dn * dist;
		return true;
	}
	return false;
}

//--------------------------------------------------------------
//creates an new sphere and pushes it onto scene vector
//
void ofApp::createSphere() {

	scene.push_back(new Sphere(glm::vec3(0, 0, 0), sphereRadius, ofColor::blue));

}


//--------------------------------------------------------------
//deletes selected sphere from scene
//
void ofApp::deleteSphere() {
	if (objSelected() && selected[0]) {
		for (int i = 0; i < scene.size(); i++)
		{
			if (scene[i] == selected[0]) {
				scene.erase(scene.begin() + i);
			}
		}
		selected.clear();
	}
}

//--------------------------------------------------------------
//creates an new light and pushes it onto scene vector
//
void ofApp::createLight() {
	aimPoint.push_back(new Sphere(glm::vec3(3, -2, 0), aimPointRadius));
	light.push_back(new Light(glm::vec3(1, 1, 1), aimPoint[numofLights]->position, .2, 10, 5));		//top left light
	numofLights++;
}


//--------------------------------------------------------------
//deletes selected light from scene
//
void ofApp::deleteLight() {
	if (objSelected() && selected[0]) {
		for (int i = 0; i < light.size(); i++)
		{
			if (light[i] == selected[0]) {
				light.erase(light.begin() + i);
				aimPoint.erase(aimPoint.begin() + i);
				numofLights--;
			}
		}
		selected.clear();
	}
}

//--------------------------------------------------------------
//ray tracing algorithm
//loops through all pixels in image
//shades each pixel with phong shading
//saves image to disk
//
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
				//get diffuse and specular
				ofColor diffuse = scene[closestIndex]->getDiffuse(r.evalPoint(close));
				ofColor specular = scene[closestIndex]->getSpecular(r.evalPoint(close));

				//add shading contribution
				closest = shade(r.evalPoint(close), scene[closestIndex]->getNormal(glm::vec3(0, 0, 0)), diffuse, close, specular, power, r);
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
//adds shading contribution
//calculates shadows
//returns shaded color
//
ofColor ofApp::shade(const glm::vec3& p, const glm::vec3& norm, const ofColor diffuse, float distance, const ofColor specular, float power, Ray r) {
	ofColor shaded = (0, 0, 0);
	glm::vec3 p1 = p;

	//loop through all lights
	for (int i = 0; i < light.size(); i++) {
		blocked = false;

		//test for shadows
		if (closestIndex < 2) {								//if the closest object is one of the planes

			for (int k = 0; k < 2; k++) {
				if (scene[k]->intersect(r, p1, glm::vec3(0, 1, 0))) {													//check if current point intersected with ground plane

					Ray shadowRay = Ray(scene[k]->getIntersectionPoint(), light[i]->position - scene[k]->getIntersectionPoint());

					//check all sphere objects
					for (int j = 2; j < scene.size(); j++) {
						if (scene[j]->intersect(shadowRay, p1, scene[j]->getNormal(p1))) {
							blocked = true;
						}
					}
				}
			}
		}
		if (!blocked) {
			//add shading contribution for current light
			//
			if (light[i]->isSpotLight) {
				shaded = spotLightPhong(p, norm, diffuse, specular, light[i]->power, distance, r, *light[i]);
			}
			else if (light[i]->isAreaLight) {
				shaded = areaLightPhong(p, norm, diffuse, specular, light[i]->power, distance, r, *light[i]);
			}
			else {
				shaded += phong(p, norm, diffuse, specular, light[i]->power, distance, r, *light[i]);
			}
		}
	}
	return shaded;
}

//--------------------------------------------------------------
//calculates all shading for point lights including:
// lambert
// phong
// ambient
//returns shaded color
//
ofColor ofApp::phong(const glm::vec3& p, const glm::vec3& norm, const ofColor diffuse, const ofColor specular, float power, float distance, Ray r, Light light) {
	ofColor phong = ofColor(0, 0, 0);
	glm::vec3 h = glm::vec3(0);

	glm::vec3 l = glm::normalize(light.position - p);
	glm::vec3 v = glm::normalize(renderCam.position - p);
	h = glm::normalize(l + v);

	float distance1 = glm::distance(light.position, p);


	phong += (ambient(diffuse)) + (lambert(p, norm, diffuse, distance1, r, light)) + (specular * (light.intensity / distance1 * distance1) * glm::pow(glm::max(zero, glm::dot(norm, h)), power));

	return phong;
}

//--------------------------------------------------------------
//calculates lambert shading for point lights
//returns shaded color
//
ofColor ofApp::lambert(const glm::vec3& p, const glm::vec3& norm, const ofColor diffuse, float distance, Ray r, Light light) {
	ofColor lambert = ofColor(0, 0, 0);
	float distance1 = glm::distance(light.position, p);

	glm::vec3 l = glm::normalize(light.position - p);
	lambert += diffuse * (light.intensity / distance1 * distance1) * (glm::max(zero, glm::dot(norm, l)));

	return lambert;
}

//--------------------------------------------------------------
//calculates all shading for spot lights including:
// lambert
// phong
//returns shaded color
//
ofColor ofApp::spotLightPhong(const glm::vec3& p, const glm::vec3& norm, const ofColor diffuse, const ofColor specular, float power, float distance, Ray r, Light light) {
	ofColor phong = ofColor(0, 0, 0);
	glm::vec3 h = glm::vec3(0);

	glm::vec3 l = glm::normalize(light.position - p);
	glm::vec3 v = glm::normalize(renderCam.position - p);
	h = glm::normalize(l + v);

	float distance1 = glm::distance(light.position, p);


	phong += (spotLightLambert(p, norm, diffuse, distance1, r, light)) + (specular * (light.intensity / distance1 * distance1) * glm::pow(glm::max(zero, glm::dot(norm, h)), power));

	return phong;
}

//--------------------------------------------------------------
//calculates lambert shading for spot lights
//returns shaded color
//
ofColor ofApp::spotLightLambert(const glm::vec3& p, const glm::vec3& norm, const ofColor diffuse, float distance, Ray r, Light light) {
	ofColor lambert = ofColor(0, 0, 0);
	glm::vec3 point, normal;

	float distance1 = glm::distance(light.position, p);


	Ray s = Ray(renderCam.position, glm::normalize(p - renderCam.position));

	//calculate angle between cone aim and current point
	glm::vec3 coneAim = glm::normalize(light.position - light.aimPoint);
	glm::vec3 pointVec = glm::normalize(light.position - p);
	float theta = glm::dot(coneAim, pointVec);
	float angle = glm::acos(theta);
	//angle = glm::degrees(angle);

	if (angle < light.coneAngle/2) {		//illuminate if p is inside spot light illumination area
		glm::vec3 l = glm::normalize(light.position - p);
		lambert += diffuse * (light.intensity / distance1 * distance1) * (glm::max(zero, glm::dot(norm, l)));
	}

	return lambert;
}

//--------------------------------------------------------------
//calculates phong shading for area lights
// lambert
// phong
//returns shaded color
//
ofColor ofApp::areaLightPhong(const glm::vec3& p, const glm::vec3& norm, const ofColor diffuse, const ofColor specular, float power, float distance, Ray r, Light light) {
	ofColor phong = ofColor(0, 0, 0);
	glm::vec3 h = glm::vec3(0);

	glm::vec3 l = glm::normalize(light.position - p);
	glm::vec3 v = glm::normalize(renderCam.position - p);
	h = glm::normalize(l + v);

	float distance1 = glm::distance(light.position, p);


	phong += (areaLightLambert(p, norm, diffuse, distance1, r, light)) + (specular * (light.intensity / distance1 * distance1) * glm::pow(glm::max(zero, glm::dot(norm, h)), power));

	return phong;
}

//--------------------------------------------------------------
//--------------------------------------------------------------
//--------------------------------------------------------------
//--------------------------------------------------------------
//--------------------------------------------------------------
//--------------------------------------------------------------
//--------------------------------------------------------------
//--------------------------------------------------------------
// 
//calculates lambert shading for area lights
//returns shaded color
//
ofColor ofApp::areaLightLambert(const glm::vec3& p, const glm::vec3& norm, const ofColor diffuse, float distance, Ray r, Light light) {
	ofColor lambert = ofColor(0, 0, 0);
	glm::vec3 point, normal;

	float distance1 = glm::distance(light.position, p);



	//change algorithm here
	//
	// 
	// 
	// 
	// 
	// 
	// 
	//



	//Ray s = Ray(renderCam.position, glm::normalize(p - renderCam.position));

	float dis = glm::distance(light.aimPoint, p);

	if (dis < light.Width) {		//if p is inside spot light illumination area
		glm::vec3 l = glm::normalize(light.position - p);
		lambert += diffuse * (light.intensity / distance1 * distance1) * (glm::max(zero, glm::dot(norm, l)));
	}



	return lambert;
}


// --------------------------------------------------------------
//calculates ambient shading
//returns shaded color
ofColor ofApp::ambient(const ofColor diffuse) {
	ofColor ambient = ofColor(0, 0, 0);

	ambient = .05 * diffuse;
	//ambient = .00 * diffuse;

	return ambient;
}

//--------------------------------------------------------------
//converts the current point on the plane to a pixel on texture map
//returns the color from the texture
ofColor Plane::textureMap(glm::vec3 p) {
	ofColor tex = ofColor(0);
	//ground plane
	if (normal == glm::vec3(0, 1, 0)) {
		float x = getIntersectionPoint().x - position.x;
		float y = getIntersectionPoint().z - position.z;

		float u = ofMap(x, position.x - getWidth() / 2, position.x + getWidth() / 2, 0, floortiles);
		float v = ofMap(y, position.z - getHeight() / 2, position.z + getHeight() / 2, 0, floortiles);

		int i = u * image.getWidth() - .5;
		int j = v * image.getHeight() - .5;

		if (i > 0 && j > 0) {
			tex = image.getColor(fmod(i, image.getWidth()), fmod(j, image.getHeight()));
		}
	}
	//wall plane
	else if (normal == glm::vec3(0, 0, 1)) {
		float x = getIntersectionPoint().x - position.x;
		float y = getIntersectionPoint().y - position.y;

		float u = ofMap(x, position.x - getWidth() / 2, position.x + getWidth() / 2, 0, walltiles);
		float v = ofMap(y, position.y - getHeight() / 2, position.y + getHeight() / 2, 0, walltiles);

		int i = u * image.getWidth() - .5;
		int j = v * image.getHeight() - .5;

		if (i > 0 && j > 0) {
			tex = image.getColor(fmod(i, image.getWidth()), fmod(j, image.getHeight()));
		}
	}
	return tex;
}

//--------------------------------------------------------------
//converts the point to a pixel on the texture specular map
//returns the specular color from the texture
ofColor Plane::specularTextureMap(glm::vec3 p) {
	ofColor tex = ofColor(0);
	//ground plane
	if (normal == glm::vec3(0, 1, 0)) {
		float x = getIntersectionPoint().x - position.x;
		float y = getIntersectionPoint().z - position.z;

		float u = ofMap(x, position.x - getWidth() / 2, position.x + getWidth() / 2, 0, floortiles);
		float v = ofMap(y, position.z - getHeight() / 2, position.z + getHeight() / 2, 0, floortiles);

		int i = u * imageSpec.getWidth() - .5;
		int j = v * imageSpec.getHeight() - .5;

		if (i > 0 && j > 0) {
			tex = imageSpec.getColor(fmod(i, imageSpec.getWidth()), fmod(j, imageSpec.getHeight()));
		}
	}
	//wall plane
	else if (normal == glm::vec3(0, 0, 1)) {
		float x = getIntersectionPoint().x - position.x;
		float y = getIntersectionPoint().y - position.y;

		float u = ofMap(x, position.x - getWidth() / 2, position.x + getWidth() / 2, 0, walltiles);
		float v = ofMap(y, position.y - getHeight() / 2, position.y + getHeight() / 2, 0, walltiles);

		int i = u * imageSpec.getWidth() - .5;
		int j = v * imageSpec.getHeight() - .5;

		if (i > 0 && j > 0) {
			tex = imageSpec.getColor(fmod(i, imageSpec.getWidth()), fmod(j, imageSpec.getHeight()));
		}
	}
	return tex;
}
