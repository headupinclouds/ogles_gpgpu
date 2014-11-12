#include "core.h"

#include <string>
#include <algorithm>

using namespace std;
using namespace ogles_gpgpu;

Core::Core() {
    initialized = false;
    useMipmaps = false;
    glExtNPOTMipmaps = false;
    inputSizeIsPOT = false;
    inputFrameW = inputFrameH = 0;
    outputFrameW = outputFrameH = 0;
    outputTexId = 0;
    firstProc = lastProc = NULL;
}

void Core::addProcToPipeline(ProcBase *proc) {
    if (initialized) {
        cerr << "ogles_gpgpu::Core - adding processor failed: pipeline already initialized" << endl;
    }
    
    cout << "ogles_gpgpu::Core - adding processor #" << (pipeline.size() + 1) << " to pipeline" << endl;
    
    pipeline.push_back(proc);
}

void Core::init(int inW, int inH, bool genInputTexId) {
    assert(!initialized && inW > 0 && inH > 0 && pipeline.size() > 0);
    
    inputSizeIsPOT = Tools::isPOT(inW) && Tools::isPOT(inH);
    inputFrameW = inW;
    inputFrameH = inH;
    
    checkGLExtensions();
    
    // init opengl
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glDisable(GL_DEPTH_TEST);
    glActiveTexture(GL_TEXTURE1);
    
    cout << "ogles_gpgpu::Core - init with input frame size "
         << inputFrameW << "x" << inputFrameH
         << " (POT: " <<  inputSizeIsPOT << ")" << endl;
    
    Tools::checkGLErr("ogles_gpgpu::Core - init");
    
    // generate input texture id if necessary
    if (genInputTexId) {
        glGenTextures(1, &inputTexId);
    } else {
        inputTexId = 0;
    }
    
    // initialize the pipeline
    ProcBase *prevProc = NULL;
    unsigned int num = 0;
    for (list<ProcBase *>::iterator it = pipeline.begin();
         it != pipeline.end();
         ++it)
    {
        // find out the input frame size for the proc
        int pipelineFrameW, pipelineFrameH;
        
        if (num == 0) {
            firstProc = *it;
            
            pipelineFrameW = inputFrameW;
            pipelineFrameH = inputFrameH;
        } else {
            pipelineFrameW = prevProc->getOutFrameW();
            pipelineFrameH = prevProc->getOutFrameH();
        }
        
        // set input texture id
        if (num > 0) {
            (*it)->useTexture(prevProc->getOutputTexId());  // chain together
        }
        
        // initialize current proc
        (*it)->init(pipelineFrameW, pipelineFrameH, num);
        
        // if this proc will downscale, we should generate a mipmap for the previous output
        if (num > 0) {
            prevProc->createFBOTex(useMipmaps && (*it)->getWillDownscale());
        }
        
        // set pointer to previous proc
        prevProc = *it;
        
        num++;
    }
    
    prevProc->createFBOTex(false);
    
    lastProc = prevProc;

    // set output texture id and size
    outputTexId = lastProc->getOutputTexId();
    outputFrameW = lastProc->getOutFrameW();
    outputFrameH = lastProc->getOutFrameH();
    
    initialized = true;
}

void Core::setInputData(const unsigned char *data) {
    assert(initialized && inputTexId > 0);
    
    if (useMipmaps && !inputSizeIsPOT && !glExtNPOTMipmaps) {
        cout << "ogles_gpgpu::Core - setInputData - WARNING: NPOT input image provided but not supported for mipmapping!" << endl;
    }
    
	// set texture
    glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, inputTexId);	// bind input texture

	// set clamping
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // copy data as texture to GPU
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, inputFrameW, inputFrameH, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    
    // mipmapping
    if (firstProc->getWillDownscale() && useMipmaps) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }
	
    
    Tools::checkGLErr("ogles_gpgpu::Core - setInputData");
    
    glFinish();
}

void Core::process() {
    assert(initialized);
    
    // set input texture id
    firstProc->useTexture(inputTexId);
    
    // set output texture id
    outputTexId = lastProc->getOutputTexId();
    
    // run the processors in the pipeline
    for (list<ProcBase *>::iterator it = pipeline.begin();
         it != pipeline.end();
         ++it)
    {
        (*it)->render();
        
        glFinish();
    }
}

void Core::getOutputData(unsigned char *buf) {
    assert(initialized);
    
    lastProc->getResultData(buf);
}

void Core::checkGLExtensions() {
    string glExtString((const char *)glGetString(GL_EXTENSIONS));
    
    vector<string> glExt = Tools::split(glExtString);
    
    for (vector<string>::iterator it = glExt.begin();
         it != glExt.end();
         ++it)
    {
        string extName = *it;
        transform(extName.begin(), extName.end(), extName.begin(), ::tolower);
        
        if (it->compare("gl_arb_texture_non_power_of_two") == 0
         || it->compare("gl_oes_texture_npot") == 0)
        {
            glExtNPOTMipmaps = true;
        }
    }
    
    cout << "ogles_gpgpu::Core - checkGLExtensions - NPOT mipmaps: " << glExtNPOTMipmaps << endl;
}