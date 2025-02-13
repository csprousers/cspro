#pragma once

#include "NameForm.h"
#include "SettingsForm.h"
#include "WildcardForm.h"
#include "PffTreeEntry.h"
#include "RunForm.h"
#include "RunData.h"

#define DEFAULT_WINDOW_TITLE			"CSPro Production Runner"
#define WILDCARD_PARAMETER				"%%parameter%%"
#define WILDCARD_PARAMETER_LEN			13
#define WILDCARD_PARAMETER_REPEAT		"%%parameterRepeat%%"
#define WILDCARD_PARAMETER_REPEAT_LEN	19

namespace CSProProductionRunner {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;


	/// <summary>
	/// Summary for Form1
	///
	/// WARNING: If you change the name of this class, you will need to change the
	///          'Resource File Name' property for the managed resource compiler tool
	///          associated with all .resx files this class depends on.  Otherwise,
	///          the designers will not be able to interact properly with localized
	///          resources associated with this form.
	/// </summary>
	public ref class Form1 : public System::Windows::Forms::Form
	{

	private: RunData^ runData;
			 enum class DoubleClickTask { TextEditor, Run, PffEditor };
			 DoubleClickTask doubleClickAction;
			 String^ pffTextEditor;
			 String^ pffEditor;
			 String^ useCSProVersion;
			 bool writePFFsUTF8;
			 bool changesMade;
			 ArrayList^ wildcardGroups;
			 ArrayList^ wildcardParameters;
			 String^ lastOpenedFilename;
	
	public:
		Form1(void)
		{
			InitializeComponent();

			runButton->Enabled = false;
			renameButton->Enabled = false;
			deleteButton->Enabled = false;
			editPFFButton->Enabled = false;
			pffEditorButton->Enabled = false;
			concurrentProcesses->Value = 1;
			writePFFsUTF8 = false;
			changesMade = false;
			runData = gcnew RunData();
			wildcardGroups = gcnew ArrayList;
			wildcardParameters = gcnew ArrayList;

			this->Text = DEFAULT_WINDOW_TITLE;
		}

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~Form1()
		{
			if (components)
			{
				delete components;
			}
		}

	private: System::Windows::Forms::Button^  runButton;
	private: System::Windows::Forms::TreeView^  pffTree;
	private: System::Windows::Forms::Button^  clearButton;
	private: System::Windows::Forms::Button^  saveButton;
	private: System::Windows::Forms::Button^  loadButton;
	private: System::Windows::Forms::Button^  addGroupButton;
	private: System::Windows::Forms::Button^  addPFFButton;
	private: System::Windows::Forms::Button^  renameButton;
	private: System::Windows::Forms::Button^  deleteButton;
	private: System::Windows::Forms::CheckBox^  shutdownBox;
	private: System::Windows::Forms::NumericUpDown^  concurrentProcesses;
	private: System::Windows::Forms::Label^  label1;
	private: System::Windows::Forms::Button^  buttonEditParameters;
	private: System::Windows::Forms::GroupBox^  groupBox1;
	private: System::Windows::Forms::Button^  settingsButton;
	private: System::Windows::Forms::Button^  editPFFButton;
	private: System::Windows::Forms::ComboBox^  comboBoxReplacementParameters;
	private: System::Windows::Forms::ComboBox^  comboBoxWildcard;
	private: System::Windows::Forms::Button^  pffEditorButton;


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
			System::ComponentModel::ComponentResourceManager^  resources = (gcnew System::ComponentModel::ComponentResourceManager(Form1::typeid));
			this->runButton = (gcnew System::Windows::Forms::Button());
			this->pffTree = (gcnew System::Windows::Forms::TreeView());
			this->clearButton = (gcnew System::Windows::Forms::Button());
			this->saveButton = (gcnew System::Windows::Forms::Button());
			this->loadButton = (gcnew System::Windows::Forms::Button());
			this->addGroupButton = (gcnew System::Windows::Forms::Button());
			this->addPFFButton = (gcnew System::Windows::Forms::Button());
			this->renameButton = (gcnew System::Windows::Forms::Button());
			this->deleteButton = (gcnew System::Windows::Forms::Button());
			this->shutdownBox = (gcnew System::Windows::Forms::CheckBox());
			this->concurrentProcesses = (gcnew System::Windows::Forms::NumericUpDown());
			this->label1 = (gcnew System::Windows::Forms::Label());
			this->buttonEditParameters = (gcnew System::Windows::Forms::Button());
			this->groupBox1 = (gcnew System::Windows::Forms::GroupBox());
			this->pffEditorButton = (gcnew System::Windows::Forms::Button());
			this->comboBoxWildcard = (gcnew System::Windows::Forms::ComboBox());
			this->editPFFButton = (gcnew System::Windows::Forms::Button());
			this->comboBoxReplacementParameters = (gcnew System::Windows::Forms::ComboBox());
			this->settingsButton = (gcnew System::Windows::Forms::Button());
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->concurrentProcesses))->BeginInit();
			this->groupBox1->SuspendLayout();
			this->SuspendLayout();
			// 
			// runButton
			// 
			this->runButton->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
			this->runButton->Location = System::Drawing::Point(806, 63);
			this->runButton->Name = L"runButton";
			this->runButton->Size = System::Drawing::Size(105, 23);
			this->runButton->TabIndex = 2;
			this->runButton->Text = L"&Run";
			this->runButton->UseVisualStyleBackColor = true;
			this->runButton->Click += gcnew System::EventHandler(this, &Form1::runButton_Click);
			// 
			// pffTree
			// 
			this->pffTree->AllowDrop = true;
			this->pffTree->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom) 
				| System::Windows::Forms::AnchorStyles::Left) 
				| System::Windows::Forms::AnchorStyles::Right));
			this->pffTree->HideSelection = false;
			this->pffTree->Location = System::Drawing::Point(12, 63);
			this->pffTree->Name = L"pffTree";
			this->pffTree->Size = System::Drawing::Size(767, 487);
			this->pffTree->TabIndex = 1;
			this->pffTree->ItemDrag += gcnew System::Windows::Forms::ItemDragEventHandler(this, &Form1::pffTree_ItemDrag);
			this->pffTree->AfterSelect += gcnew System::Windows::Forms::TreeViewEventHandler(this, &Form1::pffTree_AfterSelect);
			this->pffTree->DragDrop += gcnew System::Windows::Forms::DragEventHandler(this, &Form1::pffTree_DragDrop);
			this->pffTree->DragEnter += gcnew System::Windows::Forms::DragEventHandler(this, &Form1::pffTree_DragEnter);
			this->pffTree->DragOver += gcnew System::Windows::Forms::DragEventHandler(this, &Form1::pffTree_DragOver);
			this->pffTree->DoubleClick += gcnew System::EventHandler(this, &Form1::pffTree_DoubleClick);
			this->pffTree->KeyUp += gcnew System::Windows::Forms::KeyEventHandler(this, &Form1::pffTree_KeyUp);
			this->pffTree->MouseClick += gcnew System::Windows::Forms::MouseEventHandler(this, &Form1::pffTree_MouseClick);
			// 
			// clearButton
			// 
			this->clearButton->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
			this->clearButton->Location = System::Drawing::Point(806, 291);
			this->clearButton->Name = L"clearButton";
			this->clearButton->Size = System::Drawing::Size(105, 23);
			this->clearButton->TabIndex = 7;
			this->clearButton->Text = L"&Clear";
			this->clearButton->UseVisualStyleBackColor = true;
			this->clearButton->Click += gcnew System::EventHandler(this, &Form1::clearButton_Click);
			// 
			// saveButton
			// 
			this->saveButton->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
			this->saveButton->Location = System::Drawing::Point(806, 495);
			this->saveButton->Name = L"saveButton";
			this->saveButton->Size = System::Drawing::Size(105, 23);
			this->saveButton->TabIndex = 12;
			this->saveButton->Text = L"&Save Spec";
			this->saveButton->UseVisualStyleBackColor = true;
			this->saveButton->Click += gcnew System::EventHandler(this, &Form1::saveButton_Click);
			// 
			// loadButton
			// 
			this->loadButton->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
			this->loadButton->Location = System::Drawing::Point(806, 526);
			this->loadButton->Name = L"loadButton";
			this->loadButton->Size = System::Drawing::Size(105, 23);
			this->loadButton->TabIndex = 13;
			this->loadButton->Text = L"&Load Spec";
			this->loadButton->UseVisualStyleBackColor = true;
			this->loadButton->Click += gcnew System::EventHandler(this, &Form1::loadButton_Click);
			// 
			// addGroupButton
			// 
			this->addGroupButton->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
			this->addGroupButton->Location = System::Drawing::Point(806, 198);
			this->addGroupButton->Name = L"addGroupButton";
			this->addGroupButton->Size = System::Drawing::Size(105, 23);
			this->addGroupButton->TabIndex = 4;
			this->addGroupButton->Text = L"Add &Group";
			this->addGroupButton->UseVisualStyleBackColor = true;
			this->addGroupButton->Click += gcnew System::EventHandler(this, &Form1::addGroupButton_Click);
			// 
			// addPFFButton
			// 
			this->addPFFButton->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
			this->addPFFButton->Location = System::Drawing::Point(806, 167);
			this->addPFFButton->Name = L"addPFFButton";
			this->addPFFButton->Size = System::Drawing::Size(105, 23);
			this->addPFFButton->TabIndex = 3;
			this->addPFFButton->Text = L"&Add PFF/BAT";
			this->addPFFButton->UseVisualStyleBackColor = true;
			this->addPFFButton->Click += gcnew System::EventHandler(this, &Form1::addPFFButton_Click);
			// 
			// renameButton
			// 
			this->renameButton->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
			this->renameButton->Location = System::Drawing::Point(806, 229);
			this->renameButton->Name = L"renameButton";
			this->renameButton->Size = System::Drawing::Size(105, 23);
			this->renameButton->TabIndex = 5;
			this->renameButton->Text = L"Re&name";
			this->renameButton->UseVisualStyleBackColor = true;
			this->renameButton->Click += gcnew System::EventHandler(this, &Form1::renameButton_Click);
			// 
			// deleteButton
			// 
			this->deleteButton->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
			this->deleteButton->Location = System::Drawing::Point(806, 260);
			this->deleteButton->Name = L"deleteButton";
			this->deleteButton->Size = System::Drawing::Size(105, 23);
			this->deleteButton->TabIndex = 6;
			this->deleteButton->Text = L"&Delete";
			this->deleteButton->UseVisualStyleBackColor = true;
			this->deleteButton->Click += gcnew System::EventHandler(this, &Form1::deleteButton_Click);
			// 
			// shutdownBox
			// 
			this->shutdownBox->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
			this->shutdownBox->AutoSize = true;
			this->shutdownBox->Location = System::Drawing::Point(787, 435);
			this->shutdownBox->Name = L"shutdownBox";
			this->shutdownBox->Size = System::Drawing::Size(148, 17);
			this->shutdownBox->TabIndex = 10;
			this->shutdownBox->Text = L"Shutdown When Finished";
			this->shutdownBox->UseVisualStyleBackColor = true;
			// 
			// concurrentProcesses
			// 
			this->concurrentProcesses->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
			this->concurrentProcesses->Location = System::Drawing::Point(785, 408);
			this->concurrentProcesses->Name = L"concurrentProcesses";
			this->concurrentProcesses->Size = System::Drawing::Size(32, 20);
			this->concurrentProcesses->TabIndex = 8;
			this->concurrentProcesses->ValueChanged += gcnew System::EventHandler(this, &Form1::concurrentProcesses_ValueChanged);
			// 
			// label1
			// 
			this->label1->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
			this->label1->AutoSize = true;
			this->label1->Location = System::Drawing::Point(821, 410);
			this->label1->Name = L"label1";
			this->label1->Size = System::Drawing::Size(111, 13);
			this->label1->TabIndex = 9;
			this->label1->Text = L"Concurrent Processes";
			// 
			// buttonEditParameters
			// 
			this->buttonEditParameters->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
			this->buttonEditParameters->Location = System::Drawing::Point(481, 14);
			this->buttonEditParameters->Name = L"buttonEditParameters";
			this->buttonEditParameters->Size = System::Drawing::Size(90, 23);
			this->buttonEditParameters->TabIndex = 2;
			this->buttonEditParameters->Text = L"Edit &Parameters";
			this->buttonEditParameters->UseVisualStyleBackColor = true;
			this->buttonEditParameters->Click += gcnew System::EventHandler(this, &Form1::buttonEditParameters_Click);
			// 
			// groupBox1
			// 
			this->groupBox1->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left) 
				| System::Windows::Forms::AnchorStyles::Right));
			this->groupBox1->Controls->Add(this->pffEditorButton);
			this->groupBox1->Controls->Add(this->comboBoxWildcard);
			this->groupBox1->Controls->Add(this->editPFFButton);
			this->groupBox1->Controls->Add(this->comboBoxReplacementParameters);
			this->groupBox1->Controls->Add(this->buttonEditParameters);
			this->groupBox1->Location = System::Drawing::Point(12, 7);
			this->groupBox1->Name = L"groupBox1";
			this->groupBox1->Size = System::Drawing::Size(767, 48);
			this->groupBox1->TabIndex = 0;
			this->groupBox1->TabStop = false;
			this->groupBox1->Text = L"Wildcard Replacement Parameters";
			// 
			// pffEditorButton
			// 
			this->pffEditorButton->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
			this->pffEditorButton->Location = System::Drawing::Point(671, 14);
			this->pffEditorButton->Name = L"pffEditorButton";
			this->pffEditorButton->Size = System::Drawing::Size(90, 23);
			this->pffEditorButton->TabIndex = 4;
			this->pffEditorButton->Text = L"PF&F Editor";
			this->pffEditorButton->UseVisualStyleBackColor = true;
			this->pffEditorButton->Click += gcnew System::EventHandler(this, &Form1::pffEditorButton_Click);
			// 
			// comboBoxWildcard
			// 
			this->comboBoxWildcard->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left) 
				| System::Windows::Forms::AnchorStyles::Right));
			this->comboBoxWildcard->DropDownStyle = System::Windows::Forms::ComboBoxStyle::DropDownList;
			this->comboBoxWildcard->FormattingEnabled = true;
			this->comboBoxWildcard->Location = System::Drawing::Point(195, 16);
			this->comboBoxWildcard->Name = L"comboBoxWildcard";
			this->comboBoxWildcard->Size = System::Drawing::Size(278, 21);
			this->comboBoxWildcard->TabIndex = 1;
			// 
			// editPFFButton
			// 
			this->editPFFButton->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
			this->editPFFButton->Location = System::Drawing::Point(576, 14);
			this->editPFFButton->Name = L"editPFFButton";
			this->editPFFButton->Size = System::Drawing::Size(90, 23);
			this->editPFFButton->TabIndex = 3;
			this->editPFFButton->Text = L"Text &Edit";
			this->editPFFButton->UseVisualStyleBackColor = true;
			this->editPFFButton->Click += gcnew System::EventHandler(this, &Form1::editPFFButton_Click);
			// 
			// comboBoxReplacementParameters
			// 
			this->comboBoxReplacementParameters->DropDownStyle = System::Windows::Forms::ComboBoxStyle::DropDownList;
			this->comboBoxReplacementParameters->FormattingEnabled = true;
			this->comboBoxReplacementParameters->Location = System::Drawing::Point(12, 16);
			this->comboBoxReplacementParameters->Name = L"comboBoxReplacementParameters";
			this->comboBoxReplacementParameters->Size = System::Drawing::Size(177, 21);
			this->comboBoxReplacementParameters->TabIndex = 0;
			this->comboBoxReplacementParameters->SelectedIndexChanged += gcnew System::EventHandler(this, &Form1::comboBoxReplacementParameters_SelectedIndexChanged);
			// 
			// settingsButton
			// 
			this->settingsButton->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
			this->settingsButton->Location = System::Drawing::Point(806, 464);
			this->settingsButton->Name = L"settingsButton";
			this->settingsButton->Size = System::Drawing::Size(105, 23);
			this->settingsButton->TabIndex = 11;
			this->settingsButton->Text = L"Settings";
			this->settingsButton->UseVisualStyleBackColor = true;
			this->settingsButton->Click += gcnew System::EventHandler(this, &Form1::settingsButton_Click);
			// 
			// Form1
			// 
			this->AllowDrop = true;
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(942, 566);
			this->Controls->Add(this->settingsButton);
			this->Controls->Add(this->groupBox1);
			this->Controls->Add(this->label1);
			this->Controls->Add(this->concurrentProcesses);
			this->Controls->Add(this->shutdownBox);
			this->Controls->Add(this->deleteButton);
			this->Controls->Add(this->renameButton);
			this->Controls->Add(this->addPFFButton);
			this->Controls->Add(this->addGroupButton);
			this->Controls->Add(this->loadButton);
			this->Controls->Add(this->saveButton);
			this->Controls->Add(this->clearButton);
			this->Controls->Add(this->pffTree);
			this->Controls->Add(this->runButton);
			this->Icon = (cli::safe_cast<System::Drawing::Icon^  >(resources->GetObject(L"$this.Icon")));
			this->MinimumSize = System::Drawing::Size(950, 600);
			this->Name = L"Form1";
			this->StartPosition = System::Windows::Forms::FormStartPosition::CenterScreen;
			this->Text = L"CSPro Production Runner";
			this->FormClosing += gcnew System::Windows::Forms::FormClosingEventHandler(this, &Form1::Form1_FormClosing);
			this->Load += gcnew System::EventHandler(this, &Form1::Form1_Load);
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->concurrentProcesses))->EndInit();
			this->groupBox1->ResumeLayout(false);
			this->ResumeLayout(false);
			this->PerformLayout();

		}
#pragma endregion


private: System::Void concurrentProcesses_ValueChanged(System::Object^  sender, System::EventArgs^  e) {
			 if( concurrentProcesses->Value < 1 )
				 concurrentProcesses->Value = 1;

			 else if( concurrentProcesses->Value > 4 )
				 concurrentProcesses->Value = 4;

			 changesMade = true;
		 }

private: System::Void addPFFButton_Click(System::Object^  sender, System::EventArgs^  e) {
			 Windows::Forms::OpenFileDialog^ ofd = gcnew Windows::Forms::OpenFileDialog;

			 ofd->Filter = "Batch Files (*.pff;*.bat)|*.pff;*.bat";
			 ofd->Multiselect = true;
			 
			 if( ofd->ShowDialog() == Windows::Forms::DialogResult::OK )
			 {
				 bool addingToGroup = pffTree->SelectedNode && (( PffTreeEntry^ )pffTree->SelectedNode->Tag)->Group;
				 bool addingToParent = pffTree->SelectedNode && pffTree->SelectedNode->Parent;

				 for( int i = 0; i < ofd->FileNames->Length; i++ )
				 {
					 TreeNode^ tn = gcnew TreeNode(ofd->FileNames[i]);
					 tn->Tag = gcnew PffTreeEntry(false);

					 if( addingToGroup )
						 pffTree->SelectedNode->Nodes->Add(tn);

					 else if( addingToParent )
						 pffTree->SelectedNode->Parent->Nodes->Add(tn);

					 else
						 pffTree->Nodes->Add(tn);
				 }

				 if( addingToGroup )
				 {
					 pffTree->SelectedNode->Expand();
					 pffTree_AfterSelect(nullptr,nullptr);
				 }

				 else if( addingToParent )
				 {
					 pffTree->SelectedNode->Expand();
					 pffTree_AfterSelect(nullptr,nullptr);
				 }

				 changesMade = true;
			 }
		 }

private: System::Void addGroupButton_Click(System::Object^  sender, System::EventArgs^  e) {
			 NameForm^ nf = gcnew NameForm("");

			 if( nf->ShowDialog() == Windows::Forms::DialogResult::OK )
			 {
				 TreeNode^ tn = gcnew TreeNode(nf->GroupName);
				 tn->Tag = gcnew PffTreeEntry(true);

				 bool addingToGroup = pffTree->SelectedNode && (( PffTreeEntry^ )pffTree->SelectedNode->Tag)->Group;
				 bool addingToParent = pffTree->SelectedNode && pffTree->SelectedNode->Parent;

				 if( addingToGroup )
				 {
					 pffTree->SelectedNode->Nodes->Add(tn);
					 pffTree->SelectedNode->Expand();
				 }

				 else if( addingToParent )
				 {
					 pffTree->SelectedNode->Parent->Nodes->Add(tn);
					 pffTree->SelectedNode->Parent->Expand();
				 }

				 else
					 pffTree->Nodes->Add(tn);

				 pffTree->SelectedNode = tn;
				 
				 changesMade = true;
			 }
		 }

private: System::Void renameButton_Click(System::Object^  sender, System::EventArgs^  e) {
			 String^ text = pffTree->SelectedNode->Text;
			 bool wasConcurrent = isConcurrent(pffTree->SelectedNode);

			 if( wasConcurrent )
				 text = text->Remove(text->IndexOf(CONCURRENT_TEXT));

			 NameForm^ nf = gcnew NameForm(text);

			 if( nf->ShowDialog() == Windows::Forms::DialogResult::OK )
			 {
				 if( wasConcurrent )
					 pffTree->SelectedNode->Text = nf->GroupName + CONCURRENT_TEXT;

				 else
					 pffTree->SelectedNode->Text = nf->GroupName;

				 changesMade = true;
			 }
		 }

private: bool existsPff(TreeNodeCollection^ tnc) {
			 bool foundPff = false;

			 for( int i = 0; !foundPff && i < tnc->Count; i++ )
			 {
				 TreeNode^ tn = tnc[i];

				 if( (( PffTreeEntry^ )tn->Tag)->Group )
					 foundPff = existsPff(tn->Nodes);

				 else
					 foundPff = true;
			 }

			 return foundPff;
		 }

private: System::Void deleteButton_Click(System::Object^  sender, System::EventArgs^  e) {
			 TreeNode^ nextNode = pffTree->SelectedNode->NextNode;

			 if( nextNode == nullptr )
				 nextNode = pffTree->SelectedNode->PrevNode;

			 pffTree->Nodes->Remove(pffTree->SelectedNode);
			 renameButton->Enabled = false;
			 deleteButton->Enabled = false;
			 editPFFButton->Enabled = false;
			 pffEditorButton->Enabled = false;

			 pffTree->SelectedNode = nextNode;

			 if( nextNode )
				 pffTree_AfterSelect(nullptr,nullptr);

			 else
				 runButton->Enabled = false;

			 changesMade = true;
		 }

private: System::Void clearButton_Click(System::Object^  sender, System::EventArgs^  e) {
			 pffTree->Nodes->Clear();
			 pffTree->SelectedNode = nullptr;
			 renameButton->Enabled = false;
			 deleteButton->Enabled = false;
			 runButton->Enabled = false;
			 editPFFButton->Enabled = false;
			 pffEditorButton->Enabled = false;
			 
			 wildcardGroups->Clear();
			 wildcardParameters->Clear();
			 UpdateWildcardGroups();
			 
			 changesMade = true;
		 }

private: System::Void pffTree_AfterSelect(System::Object^  sender, System::Windows::Forms::TreeViewEventArgs^  e) {
			 bool isGroup = (( PffTreeEntry^ )pffTree->SelectedNode->Tag)->Group;
			 
			 renameButton->Enabled = isGroup;
			 deleteButton->Enabled = true;
			 editPFFButton->Enabled = !isGroup;
			 pffEditorButton->Enabled = !isGroup && isFilenamePff(pffTree->SelectedNode->Text);

			 if( isGroup )
				 runButton->Enabled = pffTree->SelectedNode->Nodes && existsPff(pffTree->SelectedNode->Nodes);

			 else
				 runButton->Enabled = true;
		 }

private: System::Void pffTree_DragEnter(System::Object^  sender, System::Windows::Forms::DragEventArgs^  e) {
			 if( e->Data->GetDataPresent(DataFormats::FileDrop) )
				 e->Effect = DragDropEffects::Copy;

			 else if( e->Data->GetDataPresent(TreeNode::typeid) )
				 e->Effect = e->AllowedEffect;

			 else
				 e->Effect = DragDropEffects::None;
		 }


private: System::Void pffTree_DragOver(System::Object^  sender, System::Windows::Forms::DragEventArgs^  e) {
			 Point mousePoint = pffTree->PointToClient(Point(e->X,e->Y));
			 pffTree->SelectedNode = pffTree->GetNodeAt(mousePoint);

			 if( e->KeyState & 8 )
				 e->Effect = DragDropEffects::Copy;

			 else
				 e->Effect = DragDropEffects::Move;
		 }

// from http://msdn.microsoft.com/en-us/library/system.windows.forms.treeview.itemdrag.aspx
private: bool ContainsNode( TreeNode^ node1, TreeNode^ node2 ) {
      // Check the parent node of the second node.
      if ( node2->Parent == nullptr )
            return false;

      if ( node2->Parent->Equals( node1 ) )
            return true;

      // If the parent node is not null or equal to the first node, 
      // call the ContainsNode method recursively using the parent of 
      // the second node.
      return ContainsNode( node1, node2->Parent );
   }


private: void addFilename(TreeNodeCollection^ tnc,String^ filename) {
			 TreeNode^ tn = gcnew TreeNode(filename);
			 tn->Tag = gcnew PffTreeEntry(false);
			 tnc->Add(tn);

			 changesMade = true;
		 }

private: void addPffRunner(TreeNodeCollection^ tnc,String^ filename) {
			 TreeNode^ oldSelection = pffTree->SelectedNode;

			 // make the group name the name of the file (minus the extension)
			 int startString = filename->LastIndexOf('\\');
			 int endString = filename->LastIndexOf(".pffRunner");
			 String^ newName = filename->Substring(startString + 1,endString - startString - 1);

			 TreeNode^ tn = gcnew TreeNode(newName);
			 tn->Tag = gcnew PffTreeEntry(true);
			 tnc->Add(tn);

			 pffTree->SelectedNode = tn;
			 LoadFile(filename);

			 pffTree->SelectedNode = oldSelection;

			 changesMade = true;
		 }

private: void addPffsInDirectory(TreeNodeCollection^ tnc,String^ path) {
			 IO::DirectoryInfo^ dir = gcnew IO::DirectoryInfo(path);

			 Array^ dirs = dir->GetDirectories();

			 for( int i = 0; i < dirs->Length; i++ )
				 addPffsInDirectory(tnc,((IO::DirectoryInfo^)dirs->GetValue(i))->FullName);

			 Array^ files = dir->GetFiles("*.pff");

			 for( int i = 0; i < files->Length; i++ )
			 {
				 String^ filename = ((IO::FileInfo^)files->GetValue(i))->FullName;

				 if( filename->EndsWith(".pff",StringComparison::CurrentCultureIgnoreCase) )
					 addFilename(tnc,filename);

				 else if( filename->EndsWith(".pffRunner",StringComparison::CurrentCultureIgnoreCase) )
					 addPffRunner(tnc,filename);
			 }
		 }

private: System::Void pffTree_DragDrop(System::Object^  sender, System::Windows::Forms::DragEventArgs^  e) {
			 Point mousePoint = pffTree->PointToClient(Point(e->X,e->Y));
			 pffTree->SelectedNode = pffTree->GetNodeAt(mousePoint);
			 
			 if( e->Data->GetDataPresent(TreeNode::typeid) )
			 {
				 TreeNode^ draggedNode = dynamic_cast<TreeNode^>(e->Data->GetData(TreeNode::typeid));
				 TreeNode^ targetNode = pffTree->SelectedNode;

				 if( !targetNode || ( !draggedNode->Equals(targetNode) && !ContainsNode(draggedNode,targetNode) ) )
				 {
					 if( e->Effect == DragDropEffects::Copy )
						draggedNode = (TreeNode^)draggedNode->Clone();

					 else
						 draggedNode->Remove();

					 if( targetNode )
					 {
						 if( (( PffTreeEntry^ )targetNode->Tag)->Group )
							 targetNode->Nodes->Add(draggedNode);

						 else // put the node after the target node (at the same level)
						 {
							 if( targetNode->Parent )
								 targetNode->Parent->Nodes->Insert(targetNode->Index,draggedNode);

							 else
								 pffTree->Nodes->Insert(targetNode->Index,draggedNode);
						 }
					 }
					 
					 else // the user dragged the node to empty space
						 pffTree->Nodes->Add(draggedNode);
					 
					 pffTree->SelectedNode = draggedNode;

					 if( e->Effect == DragDropEffects::Copy ) // on a copy we'll expand all the nodes
						 ExpandNodes(draggedNode);
				 }

				 changesMade = true;
			 }

			 else // dragging on files / directories
			 {
				 Array^ filenames = ( Array^ )e->Data->GetData(DataFormats::FileDrop);

				 bool addingToGroup = pffTree->SelectedNode && (( PffTreeEntry^ )pffTree->SelectedNode->Tag)->Group;
				 bool addingToParent = pffTree->SelectedNode && pffTree->SelectedNode->Parent;

				 TreeNodeCollection^ tnc;

				 if( addingToGroup )
					 tnc = pffTree->SelectedNode->Nodes;

				 else if( addingToParent )
					 tnc = pffTree->SelectedNode->Parent->Nodes;

				 else
					 tnc = pffTree->Nodes;

				 for( int i = 0; i < filenames->Length; i++ )
				 {
					 String^ filename = filenames->GetValue(i)->ToString();

					 if( System::IO::Directory::Exists(filename) )
						 addPffsInDirectory(tnc,filename);

					 else if( filename->EndsWith(".pff",StringComparison::CurrentCultureIgnoreCase) )
						 addFilename(tnc,filename);

					 // batch files can be dragged in but won't be added if they're in a subdirectory
					 else if( filename->EndsWith(".bat",StringComparison::CurrentCultureIgnoreCase) )
						 addFilename(tnc,filename);

					 else if( filename->EndsWith(".pffRunner",StringComparison::CurrentCultureIgnoreCase) )
						 addPffRunner(tnc,filename);
				 }

				 if( addingToGroup || addingToParent )
				 {
					 pffTree->SelectedNode->Expand();
					 pffTree_AfterSelect(nullptr,nullptr);
				 }

				 else if( pffTree->Nodes->Count )
				 {
					 pffTree->SelectedNode = pffTree->Nodes[0];
					 pffTree_AfterSelect(nullptr,nullptr);
				 }
			 }

		 }


private: System::Void pffTree_ItemDrag(System::Object^  sender, System::Windows::Forms::ItemDragEventArgs^  e) {
			 if( e->Button == ::MouseButtons::Left )
				 DoDragDrop(e->Item,DragDropEffects::Copy | DragDropEffects::Move);
		 }


#define FILE_HEADER					"[CSPro Production Runner]"
#define SHUTDOWN_MODE				"[Shutdown]"
#define PROCESSES_HEADER			"[NumberProcesses]"
#define TITLE_HEADER				"[WindowTitle]"
#define PATH_HEADER					"[PathReference]"
#define TREE_HEADER					"[Tree]"
#define GROUP_HEADER				"[Group]"
#define GROUP_FOOTER				"[EndGroup]"
#define GROUP_EXPAND_SETTING		"[Expand]"
#define WILDCARD_GROUP				"[WildcardGroup]"

private: void SaveNodes(IO::TextWriter^ tw,TreeNodeCollection^ tnc) {
			 for( int i = 0; i < tnc->Count; i++ )
			 {
				 TreeNode^ tn = tnc[i];

				 bool isGroup = (( PffTreeEntry^ )tn->Tag)->Group;
			 
				 if( isGroup )
				 {
					 tw->WriteLine(GROUP_HEADER);

					 if( tn->IsExpanded )
						 tw->WriteLine(GROUP_EXPAND_SETTING);

					 tw->WriteLine(tn->Text);

					 SaveNodes(tw,tn->Nodes);

					 tw->WriteLine(GROUP_FOOTER);
				 }

				 else
					 tw->WriteLine(tn->Text);
			 }
		 }

private: System::Void saveButton_Click(System::Object^  sender, System::EventArgs^  e) {
			 Windows::Forms::SaveFileDialog^ sfd = gcnew Windows::Forms::SaveFileDialog;

			 sfd->Filter = "CSPro Production Runner Files (*.pffRunner)|*.pffRunner";

			 if( !String::IsNullOrEmpty(lastOpenedFilename) )
			 {
				 int lastSlash = lastOpenedFilename->LastIndexOf("\\");
				 
				 if( lastSlash >= 0 )
				 {
					 sfd->InitialDirectory = lastOpenedFilename->Substring(0,lastSlash);
					 sfd->FileName = lastOpenedFilename->Substring(lastSlash + 1);
				 }
			 }
			 
			 if( sfd->ShowDialog() == Windows::Forms::DialogResult::OK )
			 {
				 lastOpenedFilename = sfd->FileName;

				 IO::TextWriter^ tw = gcnew IO::StreamWriter(sfd->FileName,false,gcnew System::Text::UTF8Encoding(true));

				 tw->WriteLine(FILE_HEADER);

				 if( shutdownBox->Checked )
					 tw->WriteLine(SHUTDOWN_MODE);

				 tw->WriteLine(PROCESSES_HEADER);
				 tw->WriteLine(concurrentProcesses->Text);

				 if( !this->Text->Equals(DEFAULT_WINDOW_TITLE) )
				 {
					 tw->WriteLine(TITLE_HEADER);
					 tw->WriteLine(this->Text);
				 }

				 String^ pathName = sfd->FileName->Substring(0,sfd->FileName->LastIndexOf('\\') + 1);

				 tw->WriteLine(PATH_HEADER);
				 tw->WriteLine(pathName);

				 tw->WriteLine(TREE_HEADER);

				 if( pffTree->Nodes )
					 SaveNodes(tw,pffTree->Nodes);

				 for( int i = 0; i < wildcardGroups->Count; i++ )
				 {
					 tw->WriteLine(WILDCARD_GROUP);
					 tw->WriteLine(wildcardGroups[i]);
					 tw->WriteLine(wildcardParameters[i]);
					 tw->WriteLine(GROUP_FOOTER);
				 }

				 tw->Close();
			 }

			 changesMade = false;
		 }


private: bool LoadNodes(IO::TextReader^ tr,TreeNodeCollection^ tnc,String^ referencePath,String^ fileName) {
			 String^ filePath = fileName->Substring(0,fileName->LastIndexOf('\\') + 1);
			 int foundAtRelativePaths = 0,notFounds = 0;

			 Stack^ tncs = gcnew Stack;
			 String^ line;

			 ArrayList^ expandCodes = gcnew ArrayList;
			 ArrayList^ tnsForExpanding = gcnew ArrayList;
			 
			 while( ( line = tr->ReadLine() ) != nullptr && !line->Equals(WILDCARD_GROUP) )
			 {
				 if( line->Equals(GROUP_HEADER) )
				 {
					 line = tr->ReadLine();
					 if( !line )
						 throw gcnew MissingMemberException();

					 bool thisGroupExpanded = line->Equals(GROUP_EXPAND_SETTING);

					 if( thisGroupExpanded )
					 {
						 line = tr->ReadLine();
						 if( !line )
							 throw gcnew MissingMemberException();
					 }

					 TreeNode^ tn = gcnew TreeNode(line);
					 tn->Tag = gcnew PffTreeEntry(true);

					 tnc->Add(tn);
					 tncs->Push(tnc);
					 tnc = tn->Nodes;

					 expandCodes->Add(thisGroupExpanded);
					 tnsForExpanding->Add(tn);
				 }

				 else if( line->Equals(GROUP_FOOTER) )
				 {
					 if( tncs->Count == 0 )
						 throw gcnew MissingMemberException();

					 tnc = (TreeNodeCollection^)tncs->Pop();
				 }

				 else // a pff
				 {
					 String^ newFilename = nullptr;

					 // see if the pff exists at the absolute path
					 if( IO::File::Exists(line) )
						 newFilename = line;

					 else // see if it exists at a relative path
					 {
						 // compare the reference path with the pff
						 int sameCharacters = 0;
						 int minLength = Math::Min(referencePath->Length,line->Length);

						 while( sameCharacters < minLength && referencePath[sameCharacters] == line[sameCharacters] )
							 sameCharacters++;

						 String^ relativePath = "";

						 String^ referenceDiff = referencePath->Substring(sameCharacters);

						 int goBack = 0;

						 if( referenceDiff->Length )
						 {
							 for( int i = 0; i < referenceDiff->Length; i++ )
								 if( referenceDiff[i] == '\\' )
									 relativePath += "..\\";
						 }

						 relativePath += line->Substring(sameCharacters);

						 newFilename = filePath + relativePath;

						 // for each ..\, replace the folder before it
						 int relativePathPos;

						 while( ( relativePathPos = newFilename->IndexOf("\\..") ) >= 0 )
						 {
							 int prevFolderStart = newFilename->Substring(0,relativePathPos)->LastIndexOf("\\");

							 if( prevFolderStart < 0 )
								 prevFolderStart = relativePathPos;

							 newFilename = newFilename->Substring(0,prevFolderStart) + newFilename->Substring(relativePathPos + 3);
						 }
						 
						 if( IO::File::Exists(newFilename) )
							 foundAtRelativePaths++;

						 else
						 {
							 notFounds++;
							 newFilename = nullptr;
						 }
					 }

					 if( newFilename )
					 {
						 TreeNode^ tn = gcnew TreeNode(newFilename);
						 tn->Tag = gcnew PffTreeEntry(false);

						 tnc->Add(tn);
					 }
				 }
			 }

			 if( tncs->Count > 0 )
				 throw gcnew MissingMemberException();

			 for( int i = 0; i < tnsForExpanding->Count; i++ )
			 {
				 TreeNode^ tn = (TreeNode^)tnsForExpanding[i];
				 if( (Boolean)expandCodes[i] )
					 tn->Expand();
			 }

			 //if( foundAtRelativePaths + notFounds )
			 if( notFounds ) // GHM 20120712 don't display the message if only relative paths changed
			 {
				 String^ message = "";

				 if( foundAtRelativePaths )
					 message += String::Format("{0} file(s) found using relative paths. ",foundAtRelativePaths);
				 
				 if( notFounds )
					 message += String::Format("{0} file(s) not found.",notFounds);

				 MessageBox::Show(message);
			 }

			 return line != nullptr && line->Equals(WILDCARD_GROUP);
		 }


private: void LoadFile(String^ filename) {
			 IO::TextReader^ tr;

			 try {
				 filename = IO::Path::GetFullPath(filename);
				 tr = gcnew IO::StreamReader(filename);
			 }
			 catch(...)
			 {
				 return;
			 }

			 try {
				 String^ pathName;
				 String^ line = tr->ReadLine();

				 shutdownBox->Checked = false;

				 if( !line->Equals(FILE_HEADER) )
					 throw gcnew MissingMemberException();

				 line = tr->ReadLine();

				 if( line->Equals(SHUTDOWN_MODE) )
				 {
					 shutdownBox->Checked = true;
					 line = tr->ReadLine();
				 }

				 if( !line->Equals(PROCESSES_HEADER) )
					 throw gcnew MissingMemberException();

				 concurrentProcesses->Text = tr->ReadLine();
				 line = tr->ReadLine();

				 if( line->Equals(TITLE_HEADER) )
				 {
					 this->Text = tr->ReadLine();
					 line = tr->ReadLine();
				 }

				 if( !line->Equals(PATH_HEADER) )
					 throw gcnew MissingMemberException();

				 pathName = tr->ReadLine();

				 line = tr->ReadLine();

				 if( !line->Equals(TREE_HEADER) )
					 throw gcnew MissingMemberException();

				 bool addingToGroup = pffTree->SelectedNode && (( PffTreeEntry^ )pffTree->SelectedNode->Tag)->Group;
				 bool addingToParent = pffTree->SelectedNode && pffTree->SelectedNode->Parent;

				 TreeNodeCollection^ tnc;

				 if( addingToGroup )
					 tnc = pffTree->SelectedNode->Nodes;

				 else if( addingToParent )
					 tnc = pffTree->SelectedNode->Parent->Nodes;

				 else
					 tnc = pffTree->Nodes;

				 if( LoadNodes(tr,tnc,pathName,filename) ) // this returns true if there are wildcard groups
				 {
					 if( !wildcardGroups->Count || MessageBox::Show("Do you want to add the wildcard groups from this file to the current groups?","Add Groups",MessageBoxButtons::YesNo) == Windows::Forms::DialogResult::Yes )
					 {
						 do
						 {
							 line = tr->ReadLine();
							 wildcardGroups->Add(line);
							 String^ parameter = "";

							 bool firstLine = true;

							 while( ( line = tr->ReadLine() ) && !line->Equals(GROUP_FOOTER) )
							 {
								 parameter += ( firstLine ? "" : "\r\n" ) + line;
								 firstLine = false;
							 }

							 if( !line->Equals(GROUP_FOOTER) )
								 throw gcnew MissingMemberException();

							 wildcardParameters->Add(parameter);

						 } while( ( line = tr->ReadLine() ) && line->Equals(WILDCARD_GROUP) );
					 }
				 }

				 tr->Close();

				 if( addingToGroup || addingToParent )
				 {
					 pffTree->SelectedNode->Expand();
					 pffTree_AfterSelect(nullptr,nullptr);
				 }

				 else if( pffTree->TopNode )
				 {
					 pffTree->SelectedNode = pffTree->TopNode;
					 pffTree_AfterSelect(nullptr,nullptr);
				 }

			 }
			 catch(System::MissingMemberException^) { // a random exception that shouldn't get thrown otherwise
				 MessageBox::Show("There was an error in the input file.");
				 tr->Close();
			 }

			 changesMade = false;

			 String^ removedGroups = "";
			 int numRemovedGroups = 0;

			 // ensure that all the groups are unique
			 for( int i = 1; i < wildcardGroups->Count; i++ )
			 {
				 for( int j = i - 1; j >= 0; j-- )
				 {
					 if( wildcardGroups[i]->Equals(wildcardGroups[j]) )
					 {
						 removedGroups += ( numRemovedGroups++ ? ", " : "" ) + (String^)wildcardGroups[j];
						 wildcardGroups->RemoveAt(i);
						 wildcardParameters->RemoveAt(i);
						 i--;
						 break;
					 }
				 }
			 }

			 if( numRemovedGroups )
			 {
				 String^ plural1 = numRemovedGroups > 1 ? "s" : "";
				 String^ plural2 = numRemovedGroups > 1 ? "were" : "was";
				 MessageBox::Show("The multiple occurrence" + plural1  + " of the group" + plural1 + " \"" + removedGroups + "\" " + plural2 + " removed.");
			 }

			 UpdateWildcardGroups();

			 lastOpenedFilename = filename;
		 }


private: System::Void loadButton_Click(System::Object^  sender, System::EventArgs^  e) {
			 Windows::Forms::OpenFileDialog^ ofd = gcnew Windows::Forms::OpenFileDialog;

			 ofd->Filter = "CSPro Production Runner Files (*.pffRunner)|*.pffRunner";
			 
			 if( ofd->ShowDialog() == Windows::Forms::DialogResult::OK )
				 LoadFile(ofd->FileName);
		 }


private: System::Void pffTree_DoubleClick(System::Object^  sender, System::EventArgs^  e) {
			 // make it so now you can only launch the Run if a PFF is clicked on
			 bool isGroup = (( PffTreeEntry^ )pffTree->SelectedNode->Tag)->Group;

			 if( !isGroup )
			 {
				 if( doubleClickAction == DoubleClickTask::Run )
					 runButton_Click(nullptr,nullptr);
				 
				 else if( doubleClickAction == DoubleClickTask::PffEditor )
					 pffEditorButton_Click(nullptr,nullptr);
				 
				 else
					 editPFFButton_Click(nullptr,nullptr);
			 }
		 }


private: System::Void flipConcurrentStatus(System::Object^  sender, System::EventArgs^  e) {
			 if( isConcurrent(pffTree->SelectedNode) )
				 pffTree->SelectedNode->Text = pffTree->SelectedNode->Text->Remove(pffTree->SelectedNode->Text->IndexOf(CONCURRENT_TEXT));

			 else
				 pffTree->SelectedNode->Text = pffTree->SelectedNode->Text + CONCURRENT_TEXT;

			 changesMade = true;
		 }


private: System::Void pffTree_MouseClick(System::Object^  sender, System::Windows::Forms::MouseEventArgs^  e) {
			 pffTree->SelectedNode = pffTree->GetNodeAt(e->Location);
			 
			 if( e->Button == Windows::Forms::MouseButtons::Right )
			 {
				 bool isGroup = (( PffTreeEntry^ )pffTree->SelectedNode->Tag)->Group;
				 
				 Windows::Forms::ContextMenu^ contextMenu = gcnew Windows::Forms::ContextMenu;
				 
				 if( !isGroup || ( pffTree->SelectedNode->Nodes && existsPff(pffTree->SelectedNode->Nodes) ) )
					 contextMenu->MenuItems->Add("Run",gcnew System::EventHandler(this,&Form1::runButton_Click));

				 if( isGroup )
				 {
					 if( isConcurrent(pffTree->SelectedNode) )
						 contextMenu->MenuItems->Add("Change to Running Sequentially",gcnew System::EventHandler(this,&Form1::flipConcurrentStatus));

					 else
						 contextMenu->MenuItems->Add("Change to Running Concurrently",gcnew System::EventHandler(this,&Form1::flipConcurrentStatus));
					 
					 contextMenu->MenuItems->Add("Add Group",gcnew System::EventHandler(this,&Form1::addGroupButton_Click));
					 contextMenu->MenuItems->Add("Rename",gcnew System::EventHandler(this,&Form1::renameButton_Click));
				 }

				 else
				 {
					 if( isFilenamePff(pffTree->SelectedNode->Text) )
						 contextMenu->MenuItems->Add("PFF Editor",gcnew System::EventHandler(this,&Form1::pffEditorButton_Click));

					 contextMenu->MenuItems->Add("Text Edit",gcnew System::EventHandler(this,&Form1::editPFFButton_Click));
				 }

				 contextMenu->MenuItems->Add("Delete",gcnew System::EventHandler(this,&Form1::deleteButton_Click));

				 contextMenu->Show(this,e->Location);
			 }
		 }


private: System::Void Form1_Load(System::Object^  sender, System::EventArgs^  e) {
			LoadRegistrySettings();

			Array^ commandArgs = Environment::GetCommandLineArgs();

			if( commandArgs->Length >= 2 )
				LoadFile((String^)commandArgs->GetValue(1));
			
			if( commandArgs->Length >= 3 && String::Equals((String^)commandArgs->GetValue(2), "/run", System::StringComparison::InvariantCultureIgnoreCase) )
			{
				if( pffTree->Nodes->Count > 0 )
				{
					// run the top-most node
					pffTree->SelectedNode = pffTree->Nodes[0];

					// when there are wildcards, run on all groups
					int all_groups_index = comboBoxReplacementParameters->Items->IndexOf("<All Groups>");

					if( all_groups_index >= 0 )
						comboBoxReplacementParameters->SelectedIndex = all_groups_index;

					runButton_Click(nullptr,nullptr);
				}
			}
		 }

private: System::Void runButton_Click(System::Object^  sender, System::EventArgs^  e) {
			 // we'll create a copy of the selected node and then expand any wildcard groups into the temporary pffs
			 TreeNode^ wildcardExpanderNode = gcnew TreeNode(pffTree->SelectedNode->Text);
			 wildcardExpanderNode->Tag = gcnew PffTreeEntry((( PffTreeEntry^ )pffTree->SelectedNode->Tag)->Group);
			 CopyNodeChildren(pffTree->SelectedNode,wildcardExpanderNode);

			 bool selectedOnlyPFF = !((( PffTreeEntry^ )pffTree->SelectedNode->Tag)->Group);

			 if( selectedOnlyPFF ) // just a PFF or BAT file, create a group to stick this node
			 {
				 TreeNode^ tn = gcnew TreeNode("");
				 tn->Tag = gcnew PffTreeEntry(true);
				 tn->Nodes->Add(wildcardExpanderNode);
				 wildcardExpanderNode = tn;
			 }

			 if( !ExpandWildcardPFFs(wildcardExpanderNode) )
				 return;

			 if( selectedOnlyPFF ) // check if it's still a single PFF, in which case we'll just launch it
			 {
				 if( wildcardExpanderNode->GetNodeCount(true) == 1 )
				 {
					 runData->LaunchPff(wildcardExpanderNode->Nodes[0]->Text,false);
					 return;
				 }
			 }
			 
			 runData->shutdown = shutdownBox->Checked;
			 runData->concurrentProcesses = int(concurrentProcesses->Value);
			 runData->tncs = wildcardExpanderNode->Nodes;
			 
			 RunForm^ rf = gcnew RunForm(runData);
			 rf->ShowDialog();
		 }

private: System::Void CopyNodeChildren(TreeNode^ fromNode,TreeNode^ toNode) {
			for( int i = 0; i < fromNode->Nodes->Count; i++ )
			{
				TreeNode^ newTn = gcnew TreeNode(fromNode->Nodes[i]->Text);
				newTn->Tag = gcnew PffTreeEntry((( PffTreeEntry^ )fromNode->Nodes[i]->Tag)->Group);
				toNode->Nodes->Add(newTn);
				CopyNodeChildren(fromNode->Nodes[i],toNode->Nodes[i]);
			}
		 }

private: System::Void LoadRegistrySettings() {
			 doubleClickAction = DoubleClickTask::Run; // default settings
			 pffTextEditor = L"C:\\Windows\\notepad.exe";
			 useCSProVersion = nullptr;

			 // favor Notepad++ if it exists
			 if( IO::File::Exists("C:\\Program Files (x86)\\Notepad++\\notepad++.exe") )
				 pffTextEditor = "C:\\Program Files (x86)\\Notepad++\\notepad++.exe";
			 
			 else if( IO::File::Exists("C:\\Program Files\\Notepad++\\notepad++.exe") )
				 pffTextEditor = "C:\\Program Files\\Notepad++\\notepad++.exe";
			 
			 try
			 {
				 doubleClickAction = (DoubleClickTask)Registry::GetValue(REGISTRY_KEY_NAME,KEY_RUNDOUBLECLICK,(int)doubleClickAction);
				 pffTextEditor = (String ^)Registry::GetValue(REGISTRY_KEY_NAME,KEY_PFFEDITOR,pffTextEditor);
				 useCSProVersion = (String ^)Registry::GetValue(REGISTRY_KEY_NAME,KEY_CSPROVERSION,useCSProVersion);
			 }
			 catch(...) { }

			 if( !SetCSProApplicationFiles() )
			 {
				 Application::Exit();
				 return;
			 }
		 }

private: System::Void settingsButton_Click(System::Object^  sender, System::EventArgs^  e) {
			 SettingsForm^ sf = gcnew SettingsForm((int)doubleClickAction,pffTextEditor,useCSProVersion);

			 if( sf->ShowDialog() == Windows::Forms::DialogResult::OK )
				 LoadRegistrySettings(); // reload the settings from registry
		 }

private: System::Void editPFFButton_Click(System::Object^  sender, System::EventArgs^  e) {
			 // see if the editor exists
			 if( !IO::File::Exists(pffTextEditor) )
				 MessageBox::Show(L"The PFF editor could not be opened. Change the editor in the settings.");

			 else
				 System::Diagnostics::Process::Start(pffTextEditor,"\"" + pffTree->SelectedNode->Text + "\"");
		 }

private: bool isFilenamePff(String^ filename) {
			 return filename->Length >= 4 && !String::Compare(filename->Substring(filename->Length - 4),".pff",true);
		 }

private: System::Void pffEditorButton_Click(System::Object^  sender, System::EventArgs^  e) {
			 // make sure they haven't selected a batch file
			 if( !isFilenamePff(pffTree->SelectedNode->Text) )
				 editPFFButton_Click(sender,e);
			 
			 // see if the editor exists
			 else if( !IO::File::Exists(pffEditor) )
				 MessageBox::Show(L"The PFF Editor could not be found.");

			 else
				 System::Diagnostics::Process::Start(pffEditor,"\"" + pffTree->SelectedNode->Text + "\"");
		 }

private: String^ GetApplicationPath(String^% missing_files, String^ path, String^ base_filename) {
            String^ filename = IO::Path::Combine(path, base_filename);

            if( !IO::File::Exists(filename) )
            {
                missing_files = ( ( missing_files->Length == 0 ) ? "" : ( missing_files + ", " ) ) +
                    IO::Path::GetFileName(filename);
            }

            return filename;
        }

private: bool SetCSProApplicationFiles() {
			// make sure that all the CSPro files needed exist
			 String^ path = Environment::GetFolderPath(Environment::SpecialFolder::ProgramFiles);

			 if( String::IsNullOrEmpty(useCSProVersion) )
			 {
				 // get the most recent version of CSPro
				 IO::DirectoryInfo^ programFilesDir = gcnew IO::DirectoryInfo(path);
				 
				 Array^ programDirs = programFilesDir->GetDirectories("CSPro*");
				 
				 if( programDirs->Length == 0 )
				 {
					 MessageBox::Show("CSPro must be installed for this application to run");
					 return false;
				 }
				 
				 path = nullptr;
				 
				 for( int i = 0; i < programDirs->Length; i++ )
				 {
					 IO::DirectoryInfo^ dir = (IO::DirectoryInfo^)programDirs->GetValue(i);
					 
					 // we don't want the cspro mobile directory messing with this
					 if( dir->FullName->ToLower()->IndexOf("mobile") < 0 )
						 if( path == nullptr || path->CompareTo(dir->FullName) < 0 )
							 path = dir->FullName;
				 }
			 
			 }

			 else
				 path += "\\" + useCSProVersion;
			
             String^ missing_files = String::Empty;

			 runData->csbatch = GetApplicationPath(missing_files, path, "CSBatch.exe");
			 runData->csconcat = GetApplicationPath(missing_files, path, "CSConcat.exe");
			 runData->csdiff = GetApplicationPath(missing_files, path, "CSDiff.exe");
			 runData->csindex = GetApplicationPath(missing_files, path, "CSIndex.exe");
			 runData->csrefmt = GetApplicationPath(missing_files, path, "CSReFmt.exe");
			 runData->cssort = GetApplicationPath(missing_files, path, "CSSort.exe");
			 runData->cstab = GetApplicationPath(missing_files, path, "CSTab.exe");
			 runData->csexport = GetApplicationPath(missing_files, path, "CSExport.exe");
			 runData->csdeploy = GetApplicationPath(missing_files, path, "CSDeploy.exe");
			 runData->csentry = GetApplicationPath(missing_files, path, "CSEntry.exe");
			 runData->excel2cspro = GetApplicationPath(missing_files, path, "Excel2CSPro.exe");
			 runData->csfreq = GetApplicationPath(missing_files, path, "CSFreq.exe");
			 runData->cspack = GetApplicationPath(missing_files, path, "CSPack.exe");
			 runData->paradataconcatenate = GetApplicationPath(missing_files, path, "ParadataConcat.exe");
			 runData->dataviewer = GetApplicationPath(missing_files, path, "DataViewer.exe");

             if( missing_files != String::Empty )
             {
				 MessageBox::Show("At least one CSPro application files is missing: " + missing_files);
				 return false;
             }

			 String^ csproVersion = runData->csbatch->Substring(runData->csbatch->LastIndexOf("CSPro ") + 6);
			 double version;
			 writePFFsUTF8 = Double::TryParse(csproVersion->Substring(0,csproVersion->IndexOf('\\')),version) && version >= 5;

			 pffEditor = path + "\\PFF Editor.exe";

			 return true;
		 }

private: System::Void Form1_FormClosing(System::Object^  sender, System::Windows::Forms::FormClosingEventArgs^  e) {
			if( changesMade )
			{
				if( MessageBox::Show("Changes were made to the application. Would you really like to exit?","Quit?",MessageBoxButtons::YesNo) == Windows::Forms::DialogResult::No )
					e->Cancel = true;
			}
		}

private: System::Void buttonEditParameters_Click(System::Object^  sender, System::EventArgs^  e) {
			WildcardForm^ wf = gcnew WildcardForm(wildcardGroups,wildcardParameters);

			if( wf->ShowDialog() == Windows::Forms::DialogResult::OK )
			{
				wildcardGroups = wf->GetWildcardGroups();
				wildcardParameters = wf->GetWildcardParameters();
				changesMade = true;
				UpdateWildcardGroups();
			}
		}

private: System::Void UpdateWildcardGroups() {
			String^ prevSelection = comboBoxReplacementParameters->SelectedIndex >= 0 ? comboBoxReplacementParameters->SelectedItem->ToString() : nullptr;

			comboBoxReplacementParameters->Items->Clear();

			comboBoxReplacementParameters->Items->Add("<No Group>");

			int indexToSelect = 0;

			for( int i = 0; i < wildcardGroups->Count; i++ )
			{
				if( i == 0 )
				{
					comboBoxReplacementParameters->Items->Add("<All Groups>");
					comboBoxReplacementParameters->Items->Add("<All Groups No Duplicates>");
				}

				comboBoxReplacementParameters->Items->Add(wildcardGroups[i]);

				if( !String::Compare((String^)wildcardGroups[i],prevSelection,true) )
					indexToSelect = i + 3;
			}

			comboBoxReplacementParameters->SelectedIndex = indexToSelect;
		}

		 
private: int wildcardGroupIteratorCount;
		 bool wildcardRecursionOverflow;
		 String^ lastSelectedWildcard;

private: System::Void comboBoxReplacementParameters_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e) {
			lastSelectedWildcard = comboBoxWildcard->SelectedIndex >= 0 ? comboBoxWildcard->SelectedItem->ToString() : nullptr;
			comboBoxWildcard->Items->Clear();
			
			if( comboBoxReplacementParameters->SelectedIndex > 0 )
			{
				comboBoxWildcard->Items->Add("<All Parameters>");
				wildcardGroupIteratorCount = 0;
				wildcardRecursionOverflow = false;

				int startPos;
				int endPos;

				if( comboBoxReplacementParameters->SelectedIndex < 3 ) // all groups
				{
					startPos = 0;
					endPos = wildcardGroups->Count - 1;
				}

				else // a specific group
					startPos = endPos = comboBoxReplacementParameters->SelectedIndex - 3;

				for( int i = startPos; i <= endPos; i++ )
				{
					if( !ValidateWildcardGroup(i) )
					{
						comboBoxWildcard->Items->Clear();
						comboBoxWildcard->Items->Add("<Invalid Wildcard Group>");
					}
				}

				if( comboBoxWildcard->Items->Count == 1 ) // only <All Wildcards>, so there were no wildcards
					comboBoxWildcard->Items->Clear();

				if( comboBoxReplacementParameters->SelectedIndex == 2 ) // all groups, no duplicates
				{
					for( int i = comboBoxWildcard->Items->Count - 1; i >= 0; i-- )
					{
						for( int j = i - 1; j >= 0; j-- )
						{
							if( comboBoxWildcard->Items[j]->ToString()->Equals(comboBoxWildcard->Items[i]->ToString()) )
							{
								comboBoxWildcard->Items->RemoveAt(i);
								break;
							}
						}
					}
				}
			}

			if( !comboBoxWildcard->Items->Count )
				comboBoxWildcard->Items->Add("<No Parameter>");

			if( comboBoxWildcard->SelectedIndex < 0 )
				comboBoxWildcard->SelectedIndex = 0;
		 }


private: bool ValidateWildcardGroup(int wildCardGroupNum) {
			String^ parameter = (String^)wildcardGroups[wildCardGroupNum];
			String^ line;

			IO::TextReader^ tr = gcnew IO::StringReader((String^)wildcardParameters[wildCardGroupNum]);

			while( line = tr->ReadLine() )
			{
				int percentPos1 = line->IndexOf("%%");
				int percentPos2;

				if( percentPos1 >= 0 && ( percentPos2 = line->IndexOf("%%",percentPos1 + 1) ) >= 0 )
				{
					String^ newGroupName = line->Substring(percentPos1 + 2,percentPos2 - percentPos1 - 2);

					// find the group
					int foundPos = -1;

					for( int i = 0; foundPos < 0 && i < wildcardGroups->Count; i++ )
					{
						if( newGroupName->Equals((String^)wildcardGroups[i]) )
							foundPos = i;
					}

					if( foundPos < 0 )
					{
						MessageBox::Show("The wildcard group \"" + newGroupName + "\" called from group \"" + wildcardGroups[wildCardGroupNum] + "\" does not exist.");
						return false;
					}

					wildcardGroupIteratorCount++;

					const int MAX_ITERATIONS = 50;

					if( wildcardGroupIteratorCount > MAX_ITERATIONS ) // recursive group embedding
					{
						wildcardRecursionOverflow = true;
						return false;
					}

					if( !ValidateWildcardGroup(foundPos) )
					{
						wildcardGroupIteratorCount--;

						if( wildcardRecursionOverflow && wildcardGroupIteratorCount == 1 ) // the original group
							MessageBox::Show("The wildcard group ends in recursive calls, starting from group \"" + wildcardGroups[wildCardGroupNum] + "\" calling group \"" + wildcardGroups[foundPos] + "\".");

						return false;
					}

					wildcardGroupIteratorCount--;
				}

				else
				{
					comboBoxWildcard->Items->Add(line);

					if( lastSelectedWildcard && lastSelectedWildcard->Equals(line) && comboBoxWildcard->SelectedIndex < 0 )
						comboBoxWildcard->SelectedIndex = comboBoxWildcard->Items->Count - 1;
				}
			}

			return true;
		}


private: bool ExpandWildcardPFFs(TreeNode^ tn) 
		{
			for( int i = 0; i < tn->Nodes->Count; i++ )
			{
				if( (( PffTreeEntry^ )tn->Nodes[i]->Tag)->Group )
				{
					if( !ExpandWildcardPFFs(tn->Nodes[i]) )
						return false;
				}

				else if( !IO::File::Exists(tn->Nodes[i]->Text) )
				{
					MessageBox::Show("The file \"" + tn->Nodes[i]->Text + "\" is missing. Cannot proceed.");
					return false;
				}
				
				else // a PFF or BAT
				{
					bool usesParameter = false;
					bool usesParameterRepeat = false;

					IO::TextReader^ tr = gcnew IO::StreamReader(tn->Nodes[i]->Text);
					String^ line;
					
					while( line = tr->ReadLine() )
					{
						if( line->IndexOf(WILDCARD_PARAMETER,StringComparison::CurrentCultureIgnoreCase) >= 0 )
							usesParameter = true;
						
						if( line->IndexOf(WILDCARD_PARAMETER_REPEAT,StringComparison::CurrentCultureIgnoreCase) >= 0 )
							usesParameterRepeat = true;						
					}

					tr->Close();

					if( usesParameter && usesParameterRepeat )
					{
						MessageBox::Show("The file \"" + tn->Nodes[i]->Text + "\" uses both wildcard parameters and wildcard repeating parameters. A file can only use one kind of parameter.");
						return false;
					}

					if( usesParameter || usesParameterRepeat ) // now create a new file for every parameter
					{
						if( comboBoxWildcard->Items->Count == 1 ) // no parameters are available
						{
							MessageBox::Show("The file \"" + tn->Nodes[i]->Text + "\" uses wildcard " + ( usesParameter ? "" : "repeating " ) + "parameters but you have not selected any.");
							return false;
						}

						String^ filename = tn->Nodes[i]->Text;

						tn->Nodes[i]->Text = tn->Text; // convert what was previously a PFF/BAT to a group
						(( PffTreeEntry^ )tn->Nodes[i]->Tag)->Group = true;

						int startParameter,endParameter;

						if( comboBoxWildcard->SelectedIndex == 0 ) // all parameters
						{
							startParameter = 1;
							endParameter = comboBoxWildcard->Items->Count - 1;
						}

						else
							startParameter = endParameter = comboBoxWildcard->SelectedIndex;

						for( int j = startParameter; j <= endParameter; j++ )
						{
							String^ newFilename = filename->Substring(0,filename->LastIndexOf('\\') + 1) + "_" + IO::Path::GetRandomFileName() + filename->Substring(filename->Length - 4);

							TreeNode^ newTn = gcnew TreeNode(newFilename);

							PffTreeEntry^ pffTreeEntry = gcnew PffTreeEntry(false);
							pffTreeEntry->TemporaryPFF = true;
							pffTreeEntry->OriginalPFFName = filename;
							pffTreeEntry->WildcardParameter = comboBoxWildcard->Items[j]->ToString();

							if( usesParameterRepeat && startParameter != endParameter )
								pffTreeEntry->WildcardParameter = "<Multiple Parameters>";

							newTn->Tag = pffTreeEntry;

							tn->Nodes[i]->Nodes->Add(newTn);

							tr = gcnew IO::StreamReader(filename);

							IO::TextWriter^ tw;

							if( writePFFsUTF8 )
								tw = gcnew IO::StreamWriter(newFilename,false,gcnew System::Text::UTF8Encoding(true));
							else
								tw = gcnew IO::StreamWriter(newFilename,false,gcnew System::Text::ASCIIEncoding());

							while( line = tr->ReadLine() )
							{
								int strPos;

								if( usesParameter )
								{
									while( ( strPos = line->IndexOf(WILDCARD_PARAMETER,StringComparison::CurrentCultureIgnoreCase) ) >= 0 )
										line = line->Substring(0,strPos) + pffTreeEntry->WildcardParameter + line->Substring(strPos + WILDCARD_PARAMETER_LEN);

									tw->WriteLine(line);
								}

								else // uses repeating parameters
								{
									bool continueLoop = true;
									String^ origLine = line;

									for( int k = startParameter; continueLoop && k <= endParameter; k++ )
									{
										line = origLine;
										continueLoop = false;

										while( ( strPos = line->IndexOf(WILDCARD_PARAMETER_REPEAT,StringComparison::CurrentCultureIgnoreCase) ) >= 0 )
										{
											continueLoop = true;
											line = line->Substring(0,strPos) + comboBoxWildcard->Items[k]->ToString() + line->Substring(strPos + WILDCARD_PARAMETER_REPEAT_LEN);
										}

										tw->WriteLine(line);										
									}
								}								
							}

							tr->Close();
							tw->Close();

							if( usesParameterRepeat ) // all the necessary work here was done in the above else
								break;
						}
					}
				}
			}

			return true;
		}

private: System::Void pffTree_KeyUp(System::Object^  sender, System::Windows::Forms::KeyEventArgs^  e) // allow the delete key to delete nodes
		{
			if( e->KeyCode == Keys::Delete && deleteButton->Enabled )
			{
				deleteButton_Click(nullptr,nullptr);
				e->Handled = true;
			}
		}

private: void ExpandNodes(TreeNode^ node)
		 {
			 for( int i = 0; i < node->Nodes->Count; i++ )
				 ExpandNodes(node->Nodes[i]);
			 
			 node->Expand();
		 }
};

}

