#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Fbo.h"

// Settings
#include "SDASettings.h"
// Session
#include "SDASession.h"
// Log
#include "SDALog.h"
// Spout
#include "CiSpoutIn.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace SophiaDigitalArt;

class SDAVisualizerApp : public App {

public:
	SDAVisualizerApp();
	void mouseMove(MouseEvent event) override;
	void mouseDown(MouseEvent event) override;
	void mouseDrag(MouseEvent event) override;
	void mouseUp(MouseEvent event) override;
	void keyDown(KeyEvent event) override;
	void keyUp(KeyEvent event) override;
	void fileDrop(FileDropEvent event) override;
	void update() override;
	void draw() override;
	void cleanup() override;
	void setUIVisibility(bool visible);
private:
	// Settings
	SDASettingsRef					mSDASettings;
	// Session
	SDASessionRef					mSDASession;
	// Log
	SDALogRef						mSDALog;
	// Spout
	SpoutIn							mSpoutIn;
	// fbo
	bool							mIsShutDown;
	Anim<float>						mRenderWindowTimer;
	void							positionRenderWindow();
	bool							mFadeInDelay;
	bool							mFlipV;
	bool							mFlipH;
	void							renderSceneToFbo();
	gl::FboRef						mFbo;
};


SDAVisualizerApp::SDAVisualizerApp()
{
	// Settings
	mSDASettings = SDASettings::create();
	// Session
	mSDASession = SDASession::create(mSDASettings);
	mSDASession->getWindowsResolution();

	mFadeInDelay = true;
	mFlipV = false;
	mFlipH = true;
	// windows
	mIsShutDown = false;
	// fbo
	gl::Fbo::Format format;
	//format.setSamples( 4 ); // uncomment this to enable 4x antialiasing
	mFbo = gl::Fbo::create(getWindowWidth(), getWindowHeight(), format.depthTexture());

	gl::enableDepthRead();
	gl::enableDepthWrite();
	//mRenderWindowTimer = 0.0f;
	//timeline().apply(&mRenderWindowTimer, 1.0f, 2.0f).finishFn([&] { positionRenderWindow(); });
	//positionRenderWindow();
}
void SDAVisualizerApp::positionRenderWindow() {
	setUIVisibility(mSDASettings->mCursorVisible);
	mSDASettings->mRenderPosXY = ivec2(mSDASettings->mRenderX, mSDASettings->mRenderY);//20141214 was 0
	setWindowPos(mSDASettings->mRenderX, mSDASettings->mRenderY);
	setWindowSize(mSDASettings->mRenderWidth, mSDASettings->mRenderHeight);
}
// Render into the FBO
void SDAVisualizerApp::renderSceneToFbo()
{
	// this will restore the old framebuffer binding when we leave this function
	// on non-OpenGL ES platforms, you can just call mFbo->unbindFramebuffer() at the end of the function
	// but this will restore the "screen" FBO on OpenGL ES, and does the right thing on both platforms
	gl::ScopedFramebuffer fbScp(mFbo);
	// clear out the FBO with black
	gl::clear(Color::black());

	// setup the viewport to match the dimensions of the FBO
	gl::ScopedViewport scpVp(ivec2(0), mFbo->getSize());

	// render the color cube
	gl::ScopedGlslProg shaderScp(gl::getStockShader(gl::ShaderDef().color()));
	gl::color(Color(1.0f, 0.5f, 0.25f));
	gl::drawColorCube(vec3(0), vec3(2.2f));
	gl::color(Color::white());
}
void SDAVisualizerApp::setUIVisibility(bool visible)
{
	if (visible)
	{
		showCursor();
	}
	else
	{
		hideCursor();
	}
}
void SDAVisualizerApp::fileDrop(FileDropEvent event)
{
	mSDASession->fileDrop(event);
}
void SDAVisualizerApp::update()
{
	if (mFadeInDelay == false) {
		mSDASession->setFloatUniformValueByIndex(mSDASettings->IFPS, getAverageFps());
		mSDASession->update();
		// render into our FBO
		renderSceneToFbo();
	}
}
void SDAVisualizerApp::cleanup()
{
	if (!mIsShutDown)
	{
		mIsShutDown = true;
		CI_LOG_V("shutdown");
		// save settings
		mSDASettings->save();
		mSDASession->save();
		quit();
	}
}
void SDAVisualizerApp::mouseMove(MouseEvent event)
{
	if (!mSDASession->handleMouseMove(event)) {
		// let your application perform its mouseMove handling here
	}
}
void SDAVisualizerApp::mouseDown(MouseEvent event)
{
	if (!mSDASession->handleMouseDown(event)) {
		// let your application perform its mouseDown handling here
		if (event.isRightDown()) { 
			// Select a sender
			// SpoutPanel.exe must be in the executable path
			mSpoutIn.getSpoutReceiver().SelectSenderPanel(); // DirectX 11 by default
		}
	}
}
void SDAVisualizerApp::mouseDrag(MouseEvent event)
{
	if (!mSDASession->handleMouseDrag(event)) {
		// let your application perform its mouseDrag handling here
	}	
}
void SDAVisualizerApp::mouseUp(MouseEvent event)
{
	if (!mSDASession->handleMouseUp(event)) {
		// let your application perform its mouseUp handling here
	}
}

void SDAVisualizerApp::keyDown(KeyEvent event)
{
	//if (!mSDASession->handleKeyDown(event)) {
		switch (event.getCode()) {
		case KeyEvent::KEY_KP_PLUS:
		case KeyEvent::KEY_DOLLAR:
		case KeyEvent::KEY_TAB:
		case KeyEvent::KEY_f:
			positionRenderWindow();
			break;
		case KeyEvent::KEY_v:
			mFlipV = !mFlipV;
			break;
		case KeyEvent::KEY_h:
			mFlipH = !mFlipH;
			break;

		case KeyEvent::KEY_ESCAPE:
			// quit the application
			quit();
			break;
		case KeyEvent::KEY_c:
			// mouse cursor and ui visibility
			mSDASettings->mCursorVisible = !mSDASettings->mCursorVisible;
			setUIVisibility(mSDASettings->mCursorVisible);
			break;
		}
	//}
}
void SDAVisualizerApp::keyUp(KeyEvent event)
{
	if (!mSDASession->handleKeyUp(event)) {
	}
}

void SDAVisualizerApp::draw()
{
	gl::clear(Color::black());
	if (mFadeInDelay) {
		mSDASettings->iAlpha = 0.0f;
		if (getElapsedFrames() > mSDASession->getFadeInDelay()) {
			mFadeInDelay = false;
			timeline().apply(&mSDASettings->iAlpha, 0.0f, 1.0f, 1.5f, EaseInCubic());
		}
	}

	//gl::draw(mSDASession->getMixTexture(), getWindowBounds());
		// setup our camera to render the cube
	CameraPersp cam(getWindowWidth(), getWindowHeight(), 60.0f);
	cam.setPerspective(60, getWindowAspectRatio(), 1, 1000);
	cam.lookAt(vec3(2.6f, 1.6f, -2.6f), vec3(0));
	gl::setMatrices(cam);
	// use the scene we rendered into the FBO as a texture
	mFbo->bindTexture();

	//// draw a cube textured with the FBO
	//{
	//	gl::ScopedGlslProg shaderScp(gl::getStockShader(gl::ShaderDef().texture()));
	//	gl::drawCube(vec3(0), vec3(2.2f));
	//}

	gl::setMatricesWindow(toPixels(getWindowSize()));
	auto tex = mSpoutIn.receiveTexture();
	if (tex) {
		// use the scene we rendered into the FBO as a texture
		//tex->bind();

		int xLeft = 0;
		int xRight = getWindowWidth();
		int yLeft = 0;
		int yRight = getWindowHeight();
		if (mFlipV) {
			yLeft = yRight;
			yRight = 0;
		}
		if (mFlipH) {
			xLeft = xRight;
			xRight = 0;	
		}
		Rectf rectangle = Rectf(xLeft, yLeft, xRight, yRight);
		// Otherwise draw the texture and fill the screen
		if (mSDASettings->mCursorVisible) {
			// show the FBO color texture in the upper left corner
			gl::draw(mFbo->getColorTexture(), Rectf(0, 0, 128, 128));
			// and draw the depth texture adjacent
			gl::draw(mFbo->getDepthTexture(), Rectf(128, 0, 256, 128));
			gl::draw(tex, Rectf(256, 0, 384, 128));
			gl::draw(tex, Rectf(512, 0, 384, 128));
			gl::draw(tex, Rectf(512, 128, 640, 0));
			// Show the user what it is receiving
			gl::ScopedBlendAlpha alpha;
			gl::enableAlphaBlending();
			gl::drawString("Receiving from: " + mSpoutIn.getSenderName(), vec2(toPixels(20), getWindowHeight() - toPixels(30)), Color(1, 1, 1), Font("Verdana", toPixels(24)));
			gl::drawString("fps: " + std::to_string((int)getAverageFps()), vec2(getWindowWidth() - toPixels(100), getWindowHeight() - toPixels(30)), Color(1, 1, 1), Font("Verdana", toPixels(24)));
			gl::drawString("RH click to select a sender", vec2(toPixels(20), getWindowHeight() - toPixels(60)), Color(1, 1, 1), Font("Verdana", toPixels(24)));
		}
		else {
			gl::draw(tex, rectangle);
		}
	}
	else {
		/*if (mSDASettings->mCursorVisible) {
			gl::ScopedBlendAlpha alpha;
			gl::enableAlphaBlending();
			gl::drawString("No sender/texture detected", vec2(toPixels(20), toPixels(20)), Color(1, 1, 1), Font("Verdana", toPixels(24)));
		}*/
	}
	getWindow()->setTitle(mSDASettings->sFps + " fps SDAViz");
}

void prepareSettings(App::Settings *settings)
{
	settings->setWindowSize(640, 480);
}

CINDER_APP(SDAVisualizerApp, RendererGl, prepareSettings)
