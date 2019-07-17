#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Fbo.h"

// Settings
#include "VDSettings.h"
// Session
#include "VDSession.h"
// Log
#include "VDLog.h"
// Spout
#include "CiSpoutIn.h"
// ndi
#include "CinderNDISender.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace VideoDromm;

class VDVisualizerApp : public App {

public:
	VDVisualizerApp();
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
	VDSettingsRef					mVDSettings;
	// Session
	VDSessionRef					mVDSession;
	// Log
	VDLogRef						mVDLog;
	// Spout
	SpoutIn							mSpoutIn;
	gl::Texture2dRef				mSpoutTexture;
	// fbo
	bool							mIsShutDown;
	Anim<float>						mRenderWindowTimer;
	void							positionRenderWindow();
	bool							mFadeInDelay;

	int								xLeft, xRight, yLeft, yRight;
	int								margin, tWidth, tHeight;
	//! fbos
	void							renderToFbo();
	gl::FboRef						mFbo;
	//! shaders
	gl::GlslProgRef					mGlsl;
	bool							mUseShader;
	// ndi
	CinderNDISender					mNDISender;
	ci::SurfaceRef 					mSurface;
};


VDVisualizerApp::VDVisualizerApp()
	: mNDISender("VDVisualizer")
{
	// Settings
	mVDSettings = VDSettings::create("Visualizer");
	// Session
	mVDSession = VDSession::create(mVDSettings);
	mVDSession->getWindowsResolution();

	mFadeInDelay = true;

	xLeft = 0;
	xRight = mVDSettings->mRenderWidth;
	yLeft = 0;
	yRight = mVDSettings->mRenderHeight;
	margin = 20;
	tWidth = mVDSettings->mFboWidth / 2;
	tHeight = mVDSettings->mFboHeight / 2;
	// windows
	mIsShutDown = false;
	// fbo
	gl::Fbo::Format format;
	//format.setSamples( 4 ); // uncomment this to enable 4x antialiasing
	mFbo = gl::Fbo::create(mVDSettings->mRenderWidth, mVDSettings->mRenderHeight, format.depthTexture());
	// ndi
	mSurface = ci::Surface::create(mVDSettings->mRenderWidth, mVDSettings->mRenderHeight, true, SurfaceChannelOrder::BGRA);

	// shader
	mUseShader = false;
	mGlsl = gl::GlslProg::create(gl::GlslProg::Format().vertex(loadAsset("passthrough.vs")).fragment(loadAsset("post.glsl")));

	gl::enableDepthRead();
	gl::enableDepthWrite();
#ifdef _DEBUG
	
#else
	mRenderWindowTimer = 0.0f;
	timeline().apply(&mRenderWindowTimer, 1.0f, 2.0f).finishFn([&] { positionRenderWindow(); });
	positionRenderWindow();

#endif  // _DEBUG
}
void VDVisualizerApp::positionRenderWindow() {
	setUIVisibility(mVDSettings->mCursorVisible);
	mVDSettings->mRenderPosXY = ivec2(mVDSettings->mRenderX, mVDSettings->mRenderY);//20141214 was 0
	setWindowPos(mVDSettings->mRenderX, mVDSettings->mRenderY);
	setWindowSize(mVDSettings->mRenderWidth, mVDSettings->mRenderHeight);
}
// Render into the FBO
void VDVisualizerApp::renderToFbo()
{
	if (mSpoutTexture) {
		// this will restore the old framebuffer binding when we leave this function
		// on non-OpenGL ES platforms, you can just call mFbo->unbindFramebuffer() at the end of the function
		// but this will restore the "screen" FBO on OpenGL ES, and does the right thing on both platforms
		gl::ScopedFramebuffer fbScp(mFbo);
		// clear out the FBO with black
		gl::clear(Color::black());

		// setup the viewport to match the dimensions of the FBO
		gl::ScopedViewport scpVp(ivec2(0), mFbo->getSize());

		// render

		// texture binding must be before ScopedGlslProg
		mSpoutTexture->bind(0);

		gl::ScopedGlslProg prog(mGlsl);

		mGlsl->uniform("iGlobalTime", (float)getElapsedSeconds());
		mGlsl->uniform("iResolution", vec3(mVDSettings->mRenderWidth, mVDSettings->mRenderHeight, 1.0));
		mGlsl->uniform("iChannel0", 0); // texture 0
		mGlsl->uniform("iExposure", 1.0f);
		mGlsl->uniform("iSobel", 1.0f);
		mGlsl->uniform("iChromatic", 1.0f);

		gl::drawSolidRect(getWindowBounds());
	}
}
void VDVisualizerApp::setUIVisibility(bool visible)
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
void VDVisualizerApp::fileDrop(FileDropEvent event)
{
	mVDSession->fileDrop(event);
}
void VDVisualizerApp::update()
{
	if (mFadeInDelay == false) {
		mVDSession->setFloatUniformValueByIndex(mVDSettings->IFPS, getAverageFps());
		mVDSession->update();
		// render into our FBO
		if (mUseShader) {
			renderToFbo();
		}
	}
}
void VDVisualizerApp::cleanup()
{
	if (!mIsShutDown)
	{
		mIsShutDown = true;
		CI_LOG_V("shutdown");
		// save settings
		mVDSettings->save();
		mVDSession->save();
		quit();
	}
}
void VDVisualizerApp::mouseMove(MouseEvent event)
{
	if (!mVDSession->handleMouseMove(event)) {
		// let your application perform its mouseMove handling here
	}
}
void VDVisualizerApp::mouseDown(MouseEvent event)
{
	if (!mVDSession->handleMouseDown(event)) {
		// let your application perform its mouseDown handling here
		if (event.isRightDown()) { 
			// Select a sender
			// SpoutPanel.exe must be in the executable path
			mSpoutIn.getSpoutReceiver().SelectSenderPanel(); // DirectX 11 by default
		}
	}
}
void VDVisualizerApp::mouseDrag(MouseEvent event)
{
	if (!mVDSession->handleMouseDrag(event)) {
		// let your application perform its mouseDrag handling here
	}	
}
void VDVisualizerApp::mouseUp(MouseEvent event)
{
	if (!mVDSession->handleMouseUp(event)) {
		// let your application perform its mouseUp handling here
	}
}

void VDVisualizerApp::keyDown(KeyEvent event)
{
#if defined( CINDER_COCOA )
	bool isModDown = event.isMetaDown();
#else // windows
	bool isModDown = event.isControlDown();
#endif
	bool isShiftDown = event.isShiftDown();

	CI_LOG_V("main keydown: " + toString(event.getCode()) + " ctrl: " + toString(isModDown) + " shift: " + toString(isShiftDown));

	if (!mVDSession->handleKeyDown(event)) {
		switch (event.getCode()) {
		case KeyEvent::KEY_KP_PLUS:

		case KeyEvent::KEY_f:
			positionRenderWindow();
			break;
		case KeyEvent::KEY_s:
			mUseShader = !mUseShader;
			break;


		case KeyEvent::KEY_c:
			// mouse cursor and ui visibility
			mVDSettings->mCursorVisible = !mVDSettings->mCursorVisible;
			setUIVisibility(mVDSettings->mCursorVisible);
			break;
		}
	}
}
void VDVisualizerApp::keyUp(KeyEvent event)
{
	if (!mVDSession->handleKeyUp(event)) {
	}
}

void VDVisualizerApp::draw()
{
	gl::clear(Color::black());
	if (mFadeInDelay) {
		mVDSettings->iAlpha = 0.0f;
		if (getElapsedFrames() > mVDSession->getFadeInDelay()) {
			mFadeInDelay = false;
			timeline().apply(&mVDSettings->iAlpha, 0.0f, 1.0f, 1.5f, EaseInCubic());
		}
	}

	xLeft = 0;
	xRight = mVDSettings->mRenderWidth;
	yLeft = 0;
	yRight = mVDSettings->mRenderHeight;
	if (mVDSettings->mFlipV) {
		yLeft = yRight;
		yRight = 0;
	}
	if (mVDSettings->mFlipH) {
		xLeft = xRight;
		xRight = 0;	
	}
	Rectf rectangle = Rectf(xLeft, yLeft, xRight, yRight);
	gl::setMatricesWindow(toPixels(getWindowSize()));
	mSpoutTexture = mSpoutIn.receiveTexture();
	if (mSpoutTexture) {
		// Otherwise draw the texture and fill the screen
		if (mVDSettings->mCursorVisible) {
			// original
			gl::draw(mSpoutTexture, Rectf(0, 0, tWidth, tHeight));
			gl::drawString("Original", vec2(toPixels(0), toPixels(tHeight)), Color(1, 1, 1), Font("Verdana", toPixels(16)));
			// flipH
			gl::draw(mSpoutTexture, Rectf(tWidth * 2 + margin, 0, tWidth + margin, tHeight));
			gl::drawString("FlipH", vec2(toPixels(tWidth + margin), toPixels(tHeight)), Color(1, 1, 1), Font("Verdana", toPixels(16)));
			// flipV
			gl::draw(mSpoutTexture, Rectf(0, tHeight * 2 + margin, tWidth, tHeight + margin));
			gl::drawString("FlipV", vec2(toPixels(0), toPixels(tHeight * 2  + margin)), Color(1, 1, 1), Font("Verdana", toPixels(16)));

			if (mUseShader) {
				// show the FBO color texture 
				gl::draw(mFbo->getColorTexture(), Rectf(tWidth + margin, tHeight + margin, tWidth * 2 + margin, tHeight * 2 + margin));
				gl::drawString("Shader", vec2(toPixels(tWidth + margin), toPixels(tHeight * 2 + margin)), Color(1, 1, 1), Font("Verdana", toPixels(16)));
			
			}
			// Show the user what it is receiving
			gl::ScopedBlendAlpha alpha;
			gl::enableAlphaBlending();
			gl::drawString("Receiving from: " + mSpoutIn.getSenderName(), vec2(toPixels(20), getWindowHeight() - toPixels(30)), Color(1, 1, 1), Font("Verdana", toPixels(24)));
			gl::drawString("fps: " + std::to_string((int)getAverageFps()), vec2(getWindowWidth() - toPixels(100), getWindowHeight() - toPixels(30)), Color(1, 1, 1), Font("Verdana", toPixels(24)));
			gl::drawString("RH click to select a sender", vec2(toPixels(20), getWindowHeight() - toPixels(60)), Color(1, 1, 1), Font("Verdana", toPixels(24)));
		}
		else {
			if (mUseShader) {
				gl::draw(mFbo->getColorTexture(), rectangle);
			}
			else {
				gl::draw(mSpoutTexture, rectangle);
				
			}
		}
		// NDI
		if (mUseShader) {
			mSurface = Surface::create(mFbo->getColorTexture()->createSource());
		}
		else {
			mSurface = Surface::create(mSpoutTexture->createSource());
		}
		long long timecode = getElapsedFrames();
		XmlTree msg{ "ci_meta", mVDSettings->sFps + " fps VDViz" };
		mNDISender.sendMetadata(msg, timecode);
		mNDISender.sendSurface(*mSurface, timecode);
	}
	else {
		if (mVDSettings->mCursorVisible) {
			gl::ScopedBlendAlpha alpha;
			gl::enableAlphaBlending();
			gl::drawString("No sender/texture detected", vec2(toPixels(20), toPixels(20)), Color(1, 1, 1), Font("Verdana", toPixels(24)));
			gl::drawString("yLeft: " + std::to_string(yLeft), vec2(getWindowWidth() - toPixels(100), getWindowHeight() - toPixels(30)), Color(1, 1, 1), Font("Verdana", toPixels(24)));

		}
	}
	
	getWindow()->setTitle(mVDSettings->sFps + " fps VDViz");
}

void prepareSettings(App::Settings *settings)
{
	settings->setWindowSize(800, 600);
	settings->setConsoleWindowEnabled();
}

CINDER_APP(VDVisualizerApp, RendererGl, prepareSettings)
