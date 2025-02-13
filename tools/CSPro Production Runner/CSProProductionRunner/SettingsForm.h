#pragma once

#define REGISTRY_KEY_NAME	"HKEY_CURRENT_USER\\Software\\U.S. Census Bureau\\Tools\\Production Runner"
#define KEY_RUNDOUBLECLICK	"RunDoubleClick"
#define KEY_PFFEDITOR		"Editor"
#define KEY_CSPROVERSION	"Version"

namespace CSProProductionRunner {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;
	using namespace Microsoft::Win32;

	/// <summary>
	/// Summary for SettingsForm
	/// </summary>
	public ref class SettingsForm : public System::Windows::Forms::Form
	{
	public:
		SettingsForm(int doubleClickAction,String^ pffTextEditor,String^ useCSProVersion)
		{
			InitializeComponent();

			radioButtonTextEdit->Checked = doubleClickAction == 0;
			radioButtonRun->Checked = doubleClickAction == 1;
			radioButtonPffEdit->Checked = doubleClickAction == 2;

			textBoxEditor->Text = pffTextEditor;

			// get the versions of CSPro on the machine
			String^ path = Environment::GetFolderPath(Environment::SpecialFolder::ProgramFiles);
			IO::DirectoryInfo^ programFilesDir = gcnew IO::DirectoryInfo(path);

			Array^ programDirs = programFilesDir->GetDirectories("CSPro*");

			comboBoxVersion->Items->Add("<Most Recent>");
			comboBoxVersion->SelectedIndex = 0;

			// put them in order starting with the newest cspro
			for( int i = programDirs->Length - 1; i >= 0; i-- )
			{
				IO::DirectoryInfo^ dir = (IO::DirectoryInfo^)programDirs->GetValue(i);

				// we don't want the cspro mobile directory messing with this
				if( dir->FullName->ToLower()->IndexOf("mobile") < 0 )
				{
					int pos = comboBoxVersion->Items->Add(dir->Name);

					if( useCSProVersion && !String::Compare(dir->Name,useCSProVersion,true) )
						comboBoxVersion->SelectedIndex = pos;
				}
			}
		}

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~SettingsForm()
		{
			if (components)
			{
				delete components;
			}
		}
	private: System::Windows::Forms::RadioButton^  radioButtonRun;
	private: System::Windows::Forms::RadioButton^  radioButtonTextEdit;



	protected: 

	protected: 

	private: System::Windows::Forms::GroupBox^  groupBox1;
	private: System::Windows::Forms::GroupBox^  groupBox2;
	private: System::Windows::Forms::ComboBox^  comboBoxVersion;
	private: System::Windows::Forms::Button^  buttonOK;
	private: System::Windows::Forms::Button^  buttonCancel;
	private: System::Windows::Forms::GroupBox^  groupBox3;
	private: System::Windows::Forms::TextBox^  textBoxEditor;
	private: System::Windows::Forms::RadioButton^  radioButtonPffEdit;









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
			this->radioButtonRun = (gcnew System::Windows::Forms::RadioButton());
			this->radioButtonTextEdit = (gcnew System::Windows::Forms::RadioButton());
			this->groupBox1 = (gcnew System::Windows::Forms::GroupBox());
			this->radioButtonPffEdit = (gcnew System::Windows::Forms::RadioButton());
			this->groupBox2 = (gcnew System::Windows::Forms::GroupBox());
			this->comboBoxVersion = (gcnew System::Windows::Forms::ComboBox());
			this->buttonOK = (gcnew System::Windows::Forms::Button());
			this->buttonCancel = (gcnew System::Windows::Forms::Button());
			this->groupBox3 = (gcnew System::Windows::Forms::GroupBox());
			this->textBoxEditor = (gcnew System::Windows::Forms::TextBox());
			this->groupBox1->SuspendLayout();
			this->groupBox2->SuspendLayout();
			this->groupBox3->SuspendLayout();
			this->SuspendLayout();
			// 
			// radioButtonRun
			// 
			this->radioButtonRun->AutoSize = true;
			this->radioButtonRun->Location = System::Drawing::Point(12, 19);
			this->radioButtonRun->Name = L"radioButtonRun";
			this->radioButtonRun->Size = System::Drawing::Size(84, 17);
			this->radioButtonRun->TabIndex = 0;
			this->radioButtonRun->TabStop = true;
			this->radioButtonRun->Text = L"Runs the file";
			this->radioButtonRun->UseVisualStyleBackColor = true;
			// 
			// radioButtonTextEdit
			// 
			this->radioButtonTextEdit->AutoSize = true;
			this->radioButtonTextEdit->Location = System::Drawing::Point(12, 61);
			this->radioButtonTextEdit->Name = L"radioButtonTextEdit";
			this->radioButtonTextEdit->Size = System::Drawing::Size(159, 17);
			this->radioButtonTextEdit->TabIndex = 2;
			this->radioButtonTextEdit->TabStop = true;
			this->radioButtonTextEdit->Text = L"Opens the file in a text editor";
			this->radioButtonTextEdit->UseVisualStyleBackColor = true;
			// 
			// groupBox1
			// 
			this->groupBox1->Controls->Add(this->radioButtonPffEdit);
			this->groupBox1->Controls->Add(this->radioButtonTextEdit);
			this->groupBox1->Controls->Add(this->radioButtonRun);
			this->groupBox1->Location = System::Drawing::Point(13, 13);
			this->groupBox1->Name = L"groupBox1";
			this->groupBox1->Size = System::Drawing::Size(223, 92);
			this->groupBox1->TabIndex = 0;
			this->groupBox1->TabStop = false;
			this->groupBox1->Text = L"Double clicking on a PFF...";
			// 
			// radioButtonPffEdit
			// 
			this->radioButtonPffEdit->AutoSize = true;
			this->radioButtonPffEdit->Location = System::Drawing::Point(12, 40);
			this->radioButtonPffEdit->Name = L"radioButtonPffEdit";
			this->radioButtonPffEdit->Size = System::Drawing::Size(171, 17);
			this->radioButtonPffEdit->TabIndex = 1;
			this->radioButtonPffEdit->TabStop = true;
			this->radioButtonPffEdit->Text = L"Opens the file in the PFF Editor";
			this->radioButtonPffEdit->UseVisualStyleBackColor = true;
			// 
			// groupBox2
			// 
			this->groupBox2->Controls->Add(this->comboBoxVersion);
			this->groupBox2->Location = System::Drawing::Point(242, 13);
			this->groupBox2->Name = L"groupBox2";
			this->groupBox2->Size = System::Drawing::Size(164, 92);
			this->groupBox2->TabIndex = 1;
			this->groupBox2->TabStop = false;
			this->groupBox2->Text = L"Use CSPro version...";
			// 
			// comboBoxVersion
			// 
			this->comboBoxVersion->DropDownStyle = System::Windows::Forms::ComboBoxStyle::DropDownList;
			this->comboBoxVersion->FormattingEnabled = true;
			this->comboBoxVersion->Location = System::Drawing::Point(10, 36);
			this->comboBoxVersion->Name = L"comboBoxVersion";
			this->comboBoxVersion->Size = System::Drawing::Size(146, 21);
			this->comboBoxVersion->TabIndex = 0;
			// 
			// buttonOK
			// 
			this->buttonOK->Location = System::Drawing::Point(123, 178);
			this->buttonOK->Name = L"buttonOK";
			this->buttonOK->Size = System::Drawing::Size(75, 23);
			this->buttonOK->TabIndex = 3;
			this->buttonOK->Text = L"OK";
			this->buttonOK->UseVisualStyleBackColor = true;
			this->buttonOK->Click += gcnew System::EventHandler(this, &SettingsForm::buttonOK_Click);
			// 
			// buttonCancel
			// 
			this->buttonCancel->DialogResult = System::Windows::Forms::DialogResult::Cancel;
			this->buttonCancel->Location = System::Drawing::Point(221, 178);
			this->buttonCancel->Name = L"buttonCancel";
			this->buttonCancel->Size = System::Drawing::Size(75, 23);
			this->buttonCancel->TabIndex = 4;
			this->buttonCancel->Text = L"Cancel";
			this->buttonCancel->UseVisualStyleBackColor = true;
			// 
			// groupBox3
			// 
			this->groupBox3->Controls->Add(this->textBoxEditor);
			this->groupBox3->Location = System::Drawing::Point(13, 115);
			this->groupBox3->Name = L"groupBox3";
			this->groupBox3->Size = System::Drawing::Size(393, 53);
			this->groupBox3->TabIndex = 2;
			this->groupBox3->TabStop = false;
			this->groupBox3->Text = L"Text Editor";
			// 
			// textBoxEditor
			// 
			this->textBoxEditor->Location = System::Drawing::Point(12, 23);
			this->textBoxEditor->Name = L"textBoxEditor";
			this->textBoxEditor->Size = System::Drawing::Size(373, 20);
			this->textBoxEditor->TabIndex = 0;
			// 
			// SettingsForm
			// 
			this->AcceptButton = this->buttonOK;
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->CancelButton = this->buttonCancel;
			this->ClientSize = System::Drawing::Size(418, 214);
			this->Controls->Add(this->groupBox3);
			this->Controls->Add(this->buttonCancel);
			this->Controls->Add(this->buttonOK);
			this->Controls->Add(this->groupBox2);
			this->Controls->Add(this->groupBox1);
			this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::FixedDialog;
			this->MaximizeBox = false;
			this->MinimizeBox = false;
			this->Name = L"SettingsForm";
			this->StartPosition = System::Windows::Forms::FormStartPosition::CenterParent;
			this->Text = L"Settings";
			this->groupBox1->ResumeLayout(false);
			this->groupBox1->PerformLayout();
			this->groupBox2->ResumeLayout(false);
			this->groupBox3->ResumeLayout(false);
			this->groupBox3->PerformLayout();
			this->ResumeLayout(false);

		}
#pragma endregion

	private: System::Void buttonOK_Click(System::Object^  sender, System::EventArgs^  e) {
				 // save the settings to the registry
				 try
				 {
					 int doubleClickAction = radioButtonTextEdit->Checked ? 0: radioButtonRun->Checked ? 1 : 2;
					 Registry::SetValue(REGISTRY_KEY_NAME,KEY_RUNDOUBLECLICK,(int)doubleClickAction);
					 Registry::SetValue(REGISTRY_KEY_NAME,KEY_CSPROVERSION,comboBoxVersion->SelectedIndex ? comboBoxVersion->SelectedItem->ToString() : "");
					 Registry::SetValue(REGISTRY_KEY_NAME,KEY_PFFEDITOR,textBoxEditor->Text);
				 }
				 catch(...) {}

				 DialogResult = Windows::Forms::DialogResult::OK;
				 Close();
			 }
};
}
