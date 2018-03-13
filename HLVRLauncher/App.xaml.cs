using System;
using System.Collections.Generic;
using System.Configuration;
using System.Data;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Threading.Tasks;
using System.Windows;

namespace HLVRLauncher
{
	/// <summary>
	/// Interaction logic for App.xaml
	/// </summary>
	public partial class App : Application
	{
		public string HLDirectory
		{
			get;
			private set;
		}

		private Dictionary<string, string> m_args = new Dictionary<string, string>();

		private void Application_Startup(object sender, StartupEventArgs e)
		{
			// Parse args
			string lastArg = "";
			foreach (string arg in e.Args)
			{
				if (arg.StartsWith("-"))
				{
					m_args.Add(arg, "");
					lastArg = arg;
				}
				else if (!string.IsNullOrEmpty(lastArg))
				{
					m_args[lastArg] = arg;
					lastArg = "";
				}
			}

			string argValue;
			if (m_args.TryGetValue("-hldir", out argValue))
			{
				HLDirectory = argValue;
			}

			if (m_args.TryGetValue("-copycdll", out argValue))
			{
				CopyCDLL(argValue);
			}

			if (m_args.TryGetValue("-copydll", out argValue))
			{
				CopyDLL(argValue);
			}

			if (m_args.TryGetValue("-copyopengl", out argValue))
			{
				CopyOpenGL(argValue);
			}

			if (m_args.ContainsKey("-startgame"))
			{
				LaunchGame();
			}
		}

		public void LaunchGame()
		{
			if (string.IsNullOrEmpty(HLDirectory))
			{
				MessageBox.Show("Half-Life's directory is not set !", "HLVR", MessageBoxButton.OK, MessageBoxImage.Warning);
				return;
			}

			string hlExePath = Path.Combine(HLDirectory, "hl.exe");

			ProcessStartInfo processStartInfo = new ProcessStartInfo();
			processStartInfo.FileName = hlExePath;
			processStartInfo.WorkingDirectory = HLDirectory;
			processStartInfo.Arguments = "-game vr -dev -env -console -insecure -nomouse -nojoy +sv_lan 1 +vr_systemType 1 +vr_renderWorldBackface 0 +map vr_test";

			Process hlProcess = Process.Start(processStartInfo);
			hlProcess.WaitForExit();
		}

		private void CopyCDLL(string clDllSourcePath)
		{
			if (!string.IsNullOrEmpty(HLDirectory) && !string.IsNullOrEmpty(clDllSourcePath))
			{
				string clDllTargetPath = Path.Combine(HLDirectory, "vr", "cl_dlls", "client.dll");
				File.Copy(clDllSourcePath, clDllTargetPath, true);
			}
		}

		private void CopyDLL(string dllSourcePath)
		{
			if (!string.IsNullOrEmpty(HLDirectory) && !string.IsNullOrEmpty(dllSourcePath))
			{
				string dllTargetPath = Path.Combine(HLDirectory, "vr", "dlls", "hl.dll");
				File.Copy(dllSourcePath, dllTargetPath, true);
			}
		}

		private void CopyOpenGL(string openGLSourcePath)
		{
			if (!string.IsNullOrEmpty(HLDirectory) && !string.IsNullOrEmpty(openGLSourcePath))
			{
				string openGLTargetPath = Path.Combine(HLDirectory, "opengl32.dll");
                if (File.Exists(openGLTargetPath))
                {
                    FileInfo fileInfo = new FileInfo(openGLTargetPath);
                    fileInfo.IsReadOnly = false;
                }
				File.Copy(openGLSourcePath, openGLTargetPath, true);
                {
                    FileInfo fileInfo = new FileInfo(openGLTargetPath);
                    fileInfo.IsReadOnly = true;
                }
            }
		}
	}
}
