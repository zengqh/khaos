// FilePacker.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <KhaosFilePack.h>
#include <KhaosFileIterator.h>
#include <KhaosStringUtil.h>

#define TASK_SHADER_COMPILE 1
#define TASK_PACK_SYS       1
#define TASK_PACK_SHADER    1

using namespace Khaos;


void _buildOneShader( const String& fileIn, const String& fileOut, int profile )
{
    static const String fxcName = 
        "\"D:\\SDK\\Microsoft DirectX SDK (June 2010)\\Utilities\\bin\\x64\\fxc.exe\"";

    const char* cmdPara = 0;

    if ( profile == 0 )
        cmdPara = "/T vs_3_0 /E main /Fo \"%s\" \"%s\"";
    else if ( profile == 1 )
        cmdPara = "/T ps_3_0 /E main /Fo \"%s\" \"%s\"";
    else
    {
        khaosAssert(0);
        return;
    }

    char cmdLine[1024] = {};
    sprintf_s( cmdLine, cmdPara, fileOut.c_str(), fileIn.c_str() );

    String fullCmd = fxcName + " " + cmdLine;

    OutputDebugStringA( fullCmd.c_str() );
    OutputDebugStringA( "\n" );

    // run
    STARTUPINFOA si = {};
    si.cb = sizeof(si);

    PROCESS_INFORMATION pi = {};

    CreateProcessA(
        0,
        &fullCmd[0],
        0,
        0,
        FALSE,
        0,
        0,
        0,
        &si,
        &pi
        );

    WaitForSingleObject( pi.hProcess, INFINITE );

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
}

struct BuildShaderOp_
{
    String m_destDir;

    bool operator()( const String& basePath, const String& relPath, const file_iterator& it )
    {
        static const String extvs = ".vs";
        static const String extps = ".ps";

        String relFileName = relPath + it.file_name();
        String fullName = basePath + relFileName;
        String outFileName = m_destDir + relFileName + "b";

        int profile = -1;

        if ( isEndWith(relFileName, extvs) )
            profile = 0;
        else if ( isEndWith(relFileName, extps) )
            profile = 1;
        else
            return true;

        _buildOneShader( fullName, outFileName, profile );
        return true;
    }
};

void _buildShaders( const String& srcDir, const String& destDir )
{
    BuildShaderOp_ op;
    op.m_destDir = destDir;
         
    travel_path( srcDir, op, false );
}

int _tmain(int argc, _TCHAR* argv[])
{
    //////////////////////////////////////////////////////////////////////////
    // 基本目录获取
    char exeName[MAX_PATH];
    GetModuleFileNameA( 0, exeName, sizeof(exeName) );

    char exePath[MAX_PATH];
    char* filePart;
    GetFullPathNameA( exeName, MAX_PATH, exePath, &filePart );
    *filePart = 0;

    String mediaPath = String(exePath) + "../Media/";

    //////////////////////////////////////////////////////////////////////////
    // 编译shader
#if TASK_SHADER_COMPILE
    _buildShaders( _MAKE_PROJ_ROOT_DIR("Samples/Media/ShaderSrc/"), mediaPath+"Shader/" );
#endif

    //////////////////////////////////////////////////////////////////////////
    // 打包system.pak
#if TASK_PACK_SYS
    {
        String packFile = mediaPath + "System.pak";
        String basePath = mediaPath + "System/";
        exportPathToPack( packFile, basePath, true );
    }
#endif

    //////////////////////////////////////////////////////////////////////////
    // 打包shader.pak
#if TASK_PACK_SHADER
    {
        String packFile = mediaPath + "Shader.pak";
        String basePath = mediaPath + "Shader/";
        exportPathToPack( packFile, basePath, true );
    }
#endif

	return 0;
}

