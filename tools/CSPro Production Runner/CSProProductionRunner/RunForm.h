#pragma once

#include "RunData.h"
#include "PffTreeEntry.h"

using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections;
using namespace System::Windows::Forms;
using namespace System::Data;
using namespace System::Drawing;
using namespace System::Diagnostics;
using namespace System::Text;
using namespace System::Threading;


namespace CSProProductionRunner {

	/// <summary>
	/// Summary for RunForm
	///
	/// WARNING: If you change the name of this class, you will need to change the
	///          'Resource File Name' property for the managed resource compiler tool
	///          associated with all .resx files this class depends on.  Otherwise,
	///          the designers will not be able to interact properly with localized
	///          resources associated with this form.
	/// </summary>
	public ref class RunForm : public System::Windows::Forms::Form
	{
	private:
		RunData^ runData;
		ArrayList^ processes;
		ArrayList^ completeNodes;
		ArrayList^ completeGroups;
		int completeRuns,expectedRuns;
		long totalTimeElapsedCompleted;
		long programTimeAtLastCompletion;
		Thread^ thread;
		bool stopThread;
		Stopwatch^ programStopwatch;
		StringBuilder^ report;

	public:
		RunForm(RunData^ sourceRunData)
		{
			InitializeComponent();

			this->closeButton->Enabled = false;

			runData = sourceRunData;

			startTimeLabel->Text = "Start Time: " + DateTime::Now.ToShortTimeString();
			progressBar->Minimum = 0;
			progressBar->Maximum = 100;

			completeRuns = 0;
			expectedRuns = countPffs(runData->tncs);
			totalTimeElapsedCompleted = 0;

			processes = gcnew ArrayList;
			completeNodes = gcnew ArrayList;
			completeGroups = gcnew ArrayList;

			report = gcnew StringBuilder;

			UpdateDisplay();

			ThreadStart^ job = gcnew ThreadStart(this,&RunForm::RunPffsThread);
			thread = gcnew Thread(job);
			stopThread = false;
			thread->Start();
		}

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~RunForm()
		{
			if (components)
			{
				delete components;
			}
		}

	protected: 
	private: System::Windows::Forms::Button^  closeButton;
	private: System::Windows::Forms::Button^  terminateButton;
	private: System::Windows::Forms::Button^  killButton;
	private: System::Windows::Forms::Label^  completeLabel;
	private: System::Windows::Forms::Label^  startTimeLabel;
	private: System::Windows::Forms::Label^  remainingTimeLabel;
	private: System::Windows::Forms::ListBox^  infoBox;
	private: System::Windows::Forms::ProgressBar^  progressBar;
	private: System::Windows::Forms::Button^  copyButton;


	private:
		/// <summary>
		/// Required designer variable.
		/// </summary>
		System::ComponentModel::Container ^components;

#pragma region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		void InitializeComponent(void)
		{
			this->progressBar = (gcnew System::Windows::Forms::ProgressBar());
			this->closeButton = (gcnew System::Windows::Forms::Button());
			this->terminateButton = (gcnew System::Windows::Forms::Button());
			this->killButton = (gcnew System::Windows::Forms::Button());
			this->completeLabel = (gcnew System::Windows::Forms::Label());
			this->startTimeLabel = (gcnew System::Windows::Forms::Label());
			this->remainingTimeLabel = (gcnew System::Windows::Forms::Label());
			this->infoBox = (gcnew System::Windows::Forms::ListBox());
			this->copyButton = (gcnew System::Windows::Forms::Button());
			this->SuspendLayout();
			// 
			// progressBar
			// 
			this->progressBar->Location = System::Drawing::Point(22, 12);
			this->progressBar->Name = L"progressBar";
			this->progressBar->Size = System::Drawing::Size(937, 23);
			this->progressBar->TabIndex = 0;
			// 
			// closeButton
			// 
			this->closeButton->Location = System::Drawing::Point(217, 413);
			this->closeButton->Name = L"closeButton";
			this->closeButton->Size = System::Drawing::Size(99, 23);
			this->closeButton->TabIndex = 0;
			this->closeButton->Text = L"Close";
			this->closeButton->UseVisualStyleBackColor = true;
			this->closeButton->Click += gcnew System::EventHandler(this, &RunForm::closeButton_Click);
			// 
			// terminateButton
			// 
			this->terminateButton->Location = System::Drawing::Point(366, 413);
			this->terminateButton->Name = L"terminateButton";
			this->terminateButton->Size = System::Drawing::Size(99, 23);
			this->terminateButton->TabIndex = 1;
			this->terminateButton->Text = L"Terminate Early";
			this->terminateButton->UseVisualStyleBackColor = true;
			this->terminateButton->Click += gcnew System::EventHandler(this, &RunForm::terminateButton_Click);
			// 
			// killButton
			// 
			this->killButton->Location = System::Drawing::Point(515, 413);
			this->killButton->Name = L"killButton";
			this->killButton->Size = System::Drawing::Size(99, 23);
			this->killButton->TabIndex = 2;
			this->killButton->Text = L"Kill Processes";
			this->killButton->UseVisualStyleBackColor = true;
			this->killButton->Click += gcnew System::EventHandler(this, &RunForm::killButton_Click);
			// 
			// completeLabel
			// 
			this->completeLabel->AutoSize = true;
			this->completeLabel->Location = System::Drawing::Point(18, 52);
			this->completeLabel->Name = L"completeLabel";
			this->completeLabel->Size = System::Drawing::Size(102, 13);
			this->completeLabel->TabIndex = 4;
			this->completeLabel->Text = L"Complete Runs: 1/2";
			// 
			// startTimeLabel
			// 
			this->startTimeLabel->AutoSize = true;
			this->startTimeLabel->Location = System::Drawing::Point(441, 52);
			this->startTimeLabel->Name = L"startTimeLabel";
			this->startTimeLabel->Size = System::Drawing::Size(99, 13);
			this->startTimeLabel->TabIndex = 5;
			this->startTimeLabel->Text = L"Start Time: 2:34 pm";
			// 
			// remainingTimeLabel
			// 
			this->remainingTimeLabel->AutoSize = true;
			this->remainingTimeLabel->Location = System::Drawing::Point(785, 52);
			this->remainingTimeLabel->Name = L"remainingTimeLabel";
			this->remainingTimeLabel->Size = System::Drawing::Size(174, 13);
			this->remainingTimeLabel->TabIndex = 6;
			this->remainingTimeLabel->Text = L"Estimated Time Remaining: 2:34:22";
			// 
			// infoBox
			// 
			this->infoBox->FormattingEnabled = true;
			this->infoBox->HorizontalScrollbar = true;
			this->infoBox->Location = System::Drawing::Point(22, 82);
			this->infoBox->Name = L"infoBox";
			this->infoBox->SelectionMode = System::Windows::Forms::SelectionMode::None;
			this->infoBox->Size = System::Drawing::Size(937, 316);
			this->infoBox->TabIndex = 7;
			// 
			// copyButton
			// 
			this->copyButton->Location = System::Drawing::Point(664, 413);
			this->copyButton->Name = L"copyButton";
			this->copyButton->Size = System::Drawing::Size(99, 23);
			this->copyButton->TabIndex = 3;
			this->copyButton->Text = L"Copy Report";
			this->copyButton->UseVisualStyleBackColor = true;
			this->copyButton->Click += gcnew System::EventHandler(this, &RunForm::copyButton_Click);
			// 
			// RunForm
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(981, 453);
			this->ControlBox = false;
			this->Controls->Add(this->copyButton);
			this->Controls->Add(this->infoBox);
			this->Controls->Add(this->remainingTimeLabel);
			this->Controls->Add(this->startTimeLabel);
			this->Controls->Add(this->completeLabel);
			this->Controls->Add(this->killButton);
			this->Controls->Add(this->terminateButton);
			this->Controls->Add(this->closeButton);
			this->Controls->Add(this->progressBar);
			this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::FixedDialog;
			this->MaximizeBox = false;
			this->MinimizeBox = false;
			this->Name = L"RunForm";
			this->ShowIcon = false;
			this->ShowInTaskbar = false;
			this->StartPosition = System::Windows::Forms::FormStartPosition::CenterParent;
			this->Text = L"CSPro Applications Running...";
			this->FormClosing += gcnew System::Windows::Forms::FormClosingEventHandler(this, &RunForm::RunForm_FormClosing);
			this->ResumeLayout(false);
			this->PerformLayout();

		}
#pragma endregion

private: System::Void closeButton_Click(System::Object^  sender, System::EventArgs^  e) {
			 Close();
		 }

private: System::Void RunForm_FormClosing(System::Object^  sender, System::Windows::Forms::FormClosingEventArgs^  e) {
			 // prevent the user from closing the form unless the close button is active
			 if( !closeButton->Enabled )
				 e->Cancel = true;
		 }


private: System::Void terminateButton_Click(System::Object^  sender, System::EventArgs^  e) {
			 stopThread = true;
			 thread->Join();

			 addToReport("Any running applications will continue but no more applications will be launched.");

			 progressBar->Value = 0;
			 killButton->Enabled = false;
			 terminateButton->Enabled = false;
			 closeButton->Enabled = true;
		 }


private: System::Void killButton_Click(System::Object^  sender, System::EventArgs^  e) {
			 stopThread = true;
			 thread->Join();
			 
			 for( int i = 0; i < processes->Count; i++ )
			 {
				 ProcessInfo^ pi = (ProcessInfo^)processes[i];
				 
				 if( pi->process && !pi->process->HasExited )
					 pi->process->Kill();
			 }

			 addToReport("All processes killed.");

			 progressBar->Value = 0;
			 killButton->Enabled = false;
			 terminateButton->Enabled = false;
			 closeButton->Enabled = true;
		 }


private: void addToReport(String^ str) {
			 infoBox->Items->Add(str);
			 report->AppendLine(str);
		 }


private: System::Void copyButton_Click(System::Object^  sender, System::EventArgs^  e) {
			 Clipboard::Clear();
			 Clipboard::SetText(report->ToString(),TextDataFormat::UnicodeText);
		 }


private: long UpdateDisplay() { // returns how long the program should wait before updating again
			 long updateWaitTime = 500;

			 completeLabel->Text = String::Format("Complete Runs: {0} / {1}",completeRuns,expectedRuns);

			 String^ timeRemaining;

			 if( completeRuns == 0 )
				 timeRemaining = "unknown";

			 else
			 {
				 double averageProcessSpeed = programTimeAtLastCompletion / completeRuns;
				 double programTimeElapsed = (double)programStopwatch->ElapsedMilliseconds;

				 double expectedCompletionTime = averageProcessSpeed * expectedRuns;

				 for( int i = 0; i < processes->Count; i++ )
				 {
					 ProcessInfo^ pi = (ProcessInfo^)processes[i];

					 // if the current node is the longest one to run, always assume that we're 80% done with it
					 if( pi->stopwatch->ElapsedMilliseconds > averageProcessSpeed )
						 expectedCompletionTime += ( pi->stopwatch->ElapsedMilliseconds - averageProcessSpeed ) / 0.80;
				 }

				 // the following division can be greater than 1 in some cases so we protect against that
				 double percentDone = Math::Min((double)1,programTimeElapsed / expectedCompletionTime);
				 double remainingTime = programTimeElapsed * ( 1 / percentDone - 1 );
				 
				 timeRemaining = millisecondsToTimeString((System::Int64)remainingTime);

				 progressBar->Value = int(percentDone * 100);

				 if( averageProcessSpeed > 10000 )
					 updateWaitTime = 5000; // for large operations we don't need to update very frequently
			 }

			 remainingTimeLabel->Text = String::Format("Estimated Time Remaining: {0}",timeRemaining);

			 return updateWaitTime;
		 }

		 
private: int countPffs(TreeNodeCollection^ tnc) {
			 int numPffs = 0;

			 for( int i = 0; i < tnc->Count; i++ )
			 {
				 TreeNode^ tn = tnc[i];

				 if( (( PffTreeEntry^ )tn->Tag)->Group )
					 numPffs += countPffs(tn->Nodes);

				 else
					 numPffs++;
			 }

			 return numPffs;
		 }


private: bool completed(TreeNode^ tn) {
			 for( int i = 0; i < completeNodes->Count; i++ )
			 {
				 if( completeNodes[i] == tn )
					 return true;
			 }

			 return false;
		 }


private: bool completedGroup(TreeNode^ tn) {
			 for( int i = 0; i < completeGroups->Count; i++ )
			 {
				 if( completeGroups[i] == tn )
					 return true;
			 }

			 if( tn->Nodes->Count == 0 )
				 return true;

			 return false;
		 }


private: bool completedOrInProcess(TreeNode^ tn) {
			 if( completed(tn) )
				 return true;

			 for( int i = 0; i < processes->Count; i++ )
			 {
				 ProcessInfo^ pi = (ProcessInfo^)processes[i];
				 
				 if( pi->tn == tn )
					 return true;
			 }

			 return false;
		 }


private: void updateFinishedGroups(TreeNode^ tn) {
			 TreeNode^ parent = tn->Parent;
			 TreeNodeCollection^ tnc = tn->Parent->Nodes;

			 if( tnc == runData->tncs )
				 return;

			 for( int i = 0; i < tnc->Count; i++ )
			 {
				 bool isGroup = (( PffTreeEntry^ )tnc[i]->Tag)->Group;

				 if( isGroup && !completedGroup(tnc[i]) )
					 return;

				 else if( !isGroup && !completed(tnc[i]) )
					 return;
			 }

			 completeGroups->Add(parent);
			 updateFinishedGroups(parent);
		 }


private: bool areDependenciesComplete(TreeNode^ tn) {
			 TreeNode^ parent = tn->Parent;

			 if( parent == nullptr )
				 return true;

			 bool parentIsConcurrent = isConcurrent(parent);

			 for( int i = 0; i < parent->Nodes->Count && parent->Nodes[i] != tn; i++ )
			 {
				 bool isGroup = (( PffTreeEntry^ )parent->Nodes[i]->Tag)->Group;

				 bool isComplete;

				 if( isGroup )
					 isComplete = completedGroup(parent->Nodes[i]);

				 else
					 isComplete = completed(parent->Nodes[i]);

				 if( parentIsConcurrent )
				 {
					 if( isComplete )
						 return true;
				 }

				 else // sequential
				 {
					 if( parent->Nodes[i + 1] == tn )
						 return isComplete;
				 }
			 }

			 if( parent->Nodes == runData->tncs )
				 return true;

			 // if we're here, then nothing in the current group has completed
			 // if the dependencies for the parent have completed, then launch the node
			 return areDependenciesComplete(parent);
		 }


private: TreeNode^ findNextNode(TreeNodeCollection^ tnc) {
			 for( int i = 0; i < tnc->Count; i++ )
			 {
				 TreeNode^ tn = tnc[i];
				 TreeNode^ tnFromGroup;

				 if( (( PffTreeEntry^ )tn->Tag)->Group )
				 {
					 if( tnFromGroup = findNextNode(tn->Nodes) )
						 return tnFromGroup;
				 }

				 // a pff that hasn't been run yet
				 else if( !completedOrInProcess(tn) && areDependenciesComplete(tn) )
					 return tn;
			 }

			 return nullptr;
		 }


private: String^ millisecondsToTimeString(System::Int64 time) {
			 int seconds = int(time / 1000);
			 int hours = seconds / 3600;
			 int minutes = ( seconds % 3600 ) / 60;
			 seconds %= 60;
			 
			 return String::Format("{0}:{1:00}:{2:00}",hours,minutes,seconds);
		 }


private: void nodeIsFinished(TreeNode^ tn) {
			 completeRuns++;
			 completeNodes->Add(tn);
			 updateFinishedGroups(tn);
			 programTimeAtLastCompletion = (long)programStopwatch->ElapsedMilliseconds;

			 if( ((PffTreeEntry^)tn->Tag)->TemporaryPFF )
				 IO::File::Delete(tn->Text);
		 }

public: void RunPffsThread() {
			bool launchNext = true;
			int numProcesses = runData->concurrentProcesses;
			bool hadConcurrentProcesses = false;

			programStopwatch = Stopwatch::StartNew();

			infoBox->Items->Clear(); // adding this prevents the following message from appearing twice
			addToReport(String::Format("Program started at {0} {1}",DateTime::Now.ToLongDateString(),DateTime::Now.ToShortTimeString()));
			addToReport("");

			while( !stopThread && completeRuns < expectedRuns )
			{
				if( processes->Count < numProcesses ) // find the next pff to run, if one is available
				{
					TreeNode^ tn = findNextNode(runData->tncs);

					if( tn ) // there was a new node available
					{
						if( processes->Count ) // a flag for later output
							hadConcurrentProcesses = true;

						ProcessInfo^ pi = gcnew ProcessInfo;

						int searchVal = completeRuns + 1;
						pi->tn = tn;

						pi->process = runData->LaunchPff(pi->tn->Text,true);

						if( pi->process )
						{
							addToReport(String::Format("Launched at {0}: {1}",DateTime::Now.ToShortTimeString(),GetDisplayString(pi->tn)));
							pi->stopwatch = Stopwatch::StartNew();
							processes->Add(pi);

						}

						else
						{
							nodeIsFinished(pi->tn);
							totalTimeElapsedCompleted++;
							addToReport(String::Format("Invalid PFF: {0}",pi->tn->Text));
							launchNext = true;
						}
					}
				}

				for( int i = 0; i < processes->Count; i++ )
				{
					ProcessInfo^ pi = (ProcessInfo^)processes[i];

					if( pi->process && pi->process->HasExited )
					{
						pi->stopwatch->Stop();

						totalTimeElapsedCompleted += long(pi->stopwatch->ElapsedMilliseconds);

						addToReport(String::Format("Completed in {0}: {1}",millisecondsToTimeString(pi->stopwatch->ElapsedMilliseconds),GetDisplayString(pi->tn)));

						nodeIsFinished(pi->tn);

						processes->RemoveAt(i);
						i--; // because we just removed an element
					}
				}

				Thread::Sleep(UpdateDisplay());
				
			}

			addToReport("");
			addToReport(String::Format("Program ended at {0} {1}",DateTime::Now.ToLongDateString(),DateTime::Now.ToShortTimeString()));
			addToReport("");

			if( completeRuns == expectedRuns )
			{
				UpdateDisplay();

				killButton->Enabled = false;
				terminateButton->Enabled = false;
				closeButton->Enabled = true;

				programStopwatch->Stop();

				addToReport(String::Format("All applications executed in {0}.",millisecondsToTimeString(programStopwatch->ElapsedMilliseconds)));

				if( hadConcurrentProcesses )
				{
					addToReport("");
					addToReport(String::Format("Running all applications sequentially might have taken up to {0}. Time saved: {1}.",millisecondsToTimeString(totalTimeElapsedCompleted),millisecondsToTimeString(Math::Max(0,long(totalTimeElapsedCompleted - programStopwatch->ElapsedMilliseconds)))));
				}

				if( runData->shutdown )
				{
					addToReport("");
					addToReport("System shutting down.");
					System::Diagnostics::Process::Start("ShutDown","/s");
				}

			}
		}


private: String^ GetDisplayString(TreeNode^ tn) {
			 PffTreeEntry^ pffTreeEntry = (PffTreeEntry^)tn->Tag;
			 
			 if( pffTreeEntry->TemporaryPFF )
				 return pffTreeEntry->OriginalPFFName + " (parameter: " + pffTreeEntry->WildcardParameter + ")";

			 return tn->Text;
		 }

};
}
