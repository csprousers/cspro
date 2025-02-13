#pragma once

namespace CSProProductionRunner {

#define CONCURRENT_TEXT " <concurrent>"

	using namespace System;
	using namespace System::Diagnostics;
	using namespace System::Windows::Forms;
	using namespace Microsoft::Win32;
	using namespace System::Security::Permissions;

	//[assembly: RegistryPermissionAttribute(SecurityAction::RequestMinimum,All = "HKEY_LOCAL_MACHINE")];

	public ref class RunData {
	public:
		String^ csbatch;
		String^ csconcat;
		String^ csdiff;
		String^ csindex;
		String^ csrefmt;
		String^ cssort;
		String^ cstab;
		String^ csexport;
		String^ csdeploy;
		String^ csentry;
		String^ excel2cspro;
		String^ csfreq;
		String^ cspack;
		String^ paradataconcatenate;
		String^ dataviewer;
		Windows::Forms::TreeNodeCollection^ tncs;
		bool shutdown;
		int concurrentProcesses;

		Process^ LaunchPff(String^ filename,bool minimized)
		{
			if( !IO::File::Exists(filename) )
				return nullptr;

			ProcessStartInfo^ startInfo = nullptr;

			if( filename->Length >= 4 && !String::Compare(filename->Substring(filename->Length - 4),".bat",true) )
			{
				// the autorun trick initially used below doesn't work on Windows 7 so I must do it in another way

				String^ tempBatchFilename = System::IO::Path::GetTempPath() + System::IO::Path::GetRandomFileName() + ".bat";

				try
				{
					IO::TextWriter^ tw = gcnew IO::StreamWriter(tempBatchFilename);
					tw->WriteLine(String::Format("cd /d \"{0}\"",filename->Substring(0,filename->LastIndexOf('\\'))));
					tw->WriteLine(String::Format("\"{0}\"",filename->Substring(filename->LastIndexOf('\\') + 1)));
					tw->Close();
				}
				catch(...) { return nullptr; }

				startInfo = gcnew ProcessStartInfo("cmd");
				startInfo->Arguments = String::Format("/C \"{0}\"",tempBatchFilename);

				if( minimized )
					startInfo->WindowStyle = ProcessWindowStyle::Minimized;

				Process^ newProcess = Process::Start(startInfo);

				return newProcess;
			}

			// else (PFF files)

			IO::TextReader^ tr = gcnew IO::StreamReader(filename);
			
			String^ line = tr->ReadLine();

			if( !line || !line->Equals("[Run Information]") )
				return nullptr;

			line = tr->ReadLine();

			if( !line || !line->Contains("Version=") )
				return nullptr;

			line = tr->ReadLine();

			if( !line || !line->Contains("AppType=") )
				return nullptr;

			String^ appType = line->Substring(line->IndexOf('=') + 1);

			tr->Close();
			
			if( !String::Compare(appType,"Batch",true) )
				startInfo = gcnew ProcessStartInfo(csbatch);

			else if( !String::Compare(appType,"Concatenate",true) )
				startInfo = gcnew ProcessStartInfo(csconcat);

			else if( !String::Compare(appType,"Compare",true) )
				startInfo = gcnew ProcessStartInfo(csdiff);

			else if( !String::Compare(appType,"Index",true) )
				startInfo = gcnew ProcessStartInfo(csindex);

			else if( !String::Compare(appType,"Reformat",true) )
				startInfo = gcnew ProcessStartInfo(csrefmt);

			else if( !String::Compare(appType,"Sort",true) )
				startInfo = gcnew ProcessStartInfo(cssort);

			else if( !String::Compare(appType,"Tabulation",true) )
				startInfo = gcnew ProcessStartInfo(cstab);

			else if( !String::Compare(appType,"Export",true) )
				startInfo = gcnew ProcessStartInfo(csexport);

			else if( !String::Compare(appType,"Deploy",true) )
				startInfo = gcnew ProcessStartInfo(csdeploy);

			else if( !String::Compare(appType,"Entry",true) )
				startInfo = gcnew ProcessStartInfo(csentry);

			else if( !String::Compare(appType,"Excel2CSPro",true) )
				startInfo = gcnew ProcessStartInfo(excel2cspro);

			else if( !String::Compare(appType,"Frequencies",true) )
				startInfo = gcnew ProcessStartInfo(csfreq);

			else if( !String::Compare(appType,"Pack",true) )
				startInfo = gcnew ProcessStartInfo(cspack);

			else if( !String::Compare(appType,"ParadataConcatenate",true) )
				startInfo = gcnew ProcessStartInfo(paradataconcatenate);

			else if( !String::Compare(appType,"Sync",true) )
				startInfo = gcnew ProcessStartInfo(dataviewer);

			else
				return nullptr;

			startInfo->Arguments = String::Format("\"{0}\"",filename);

			if( minimized )
				startInfo->WindowStyle = ProcessWindowStyle::Minimized;

			return Process::Start(startInfo);
		}

	};

	public ref class ProcessInfo {
	public:
		Process^ process;
		Stopwatch^ stopwatch;
		TreeNode^ tn;
	};


	bool isConcurrent(TreeNode^ tn);
}