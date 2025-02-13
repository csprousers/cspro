#pragma once

#include "NameForm.h"

namespace CSProProductionRunner {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;

	/// <summary>
	/// Summary for WildcardForm
	/// </summary>
	public ref class WildcardForm : public System::Windows::Forms::Form
	{
		ArrayList^ wildcardGroups;
		ArrayList^ wildcardParameters;
		int lastSelection;

	public:
		ArrayList^ GetWildcardGroups() { return wildcardGroups; }
		ArrayList^ GetWildcardParameters() { return wildcardParameters; }

		WildcardForm(ArrayList^ in_wildcardGroups,ArrayList^ in_wildcardParameters)
		{
			InitializeComponent();

			wildcardGroups = gcnew ArrayList(in_wildcardGroups);
			wildcardParameters = gcnew ArrayList(in_wildcardParameters);
			buttonRename->Enabled = false;
			buttonDelete->Enabled = false;
			buttonMove->Enabled = false;
			lastSelection = -1;

			for( int i = 0; i < wildcardGroups->Count; i++ )
				listBoxGroups->Items->Add(wildcardGroups[i]);

			if( wildcardGroups->Count )
				listBoxGroups->SelectedIndex = 0;
			
			else
				buttonHelp_Click(nullptr,nullptr);
		}

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~WildcardForm()
		{
			if (components)
			{
				delete components;
			}
		}
	private: System::Windows::Forms::ListBox^  listBoxGroups;
	private: System::Windows::Forms::Label^  label1;
	private: System::Windows::Forms::TextBox^  textBoxWildcards;
	private: System::Windows::Forms::Label^  label2;
	private: System::Windows::Forms::Button^  buttonAdd;
	private: System::Windows::Forms::Button^  buttonRename;
	private: System::Windows::Forms::Button^  buttonDelete;
	private: System::Windows::Forms::Button^  buttonOK;
	private: System::Windows::Forms::Button^  buttonCancel;
	private: System::Windows::Forms::Button^  buttonHelp;
	private: System::Windows::Forms::Button^  buttonMove;

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
			this->listBoxGroups = (gcnew System::Windows::Forms::ListBox());
			this->label1 = (gcnew System::Windows::Forms::Label());
			this->textBoxWildcards = (gcnew System::Windows::Forms::TextBox());
			this->label2 = (gcnew System::Windows::Forms::Label());
			this->buttonAdd = (gcnew System::Windows::Forms::Button());
			this->buttonRename = (gcnew System::Windows::Forms::Button());
			this->buttonDelete = (gcnew System::Windows::Forms::Button());
			this->buttonOK = (gcnew System::Windows::Forms::Button());
			this->buttonCancel = (gcnew System::Windows::Forms::Button());
			this->buttonHelp = (gcnew System::Windows::Forms::Button());
			this->buttonMove = (gcnew System::Windows::Forms::Button());
			this->SuspendLayout();
			// 
			// listBoxGroups
			// 
			this->listBoxGroups->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom) 
				| System::Windows::Forms::AnchorStyles::Left));
			this->listBoxGroups->FormattingEnabled = true;
			this->listBoxGroups->Location = System::Drawing::Point(12, 31);
			this->listBoxGroups->Name = L"listBoxGroups";
			this->listBoxGroups->Size = System::Drawing::Size(146, 212);
			this->listBoxGroups->TabIndex = 0;
			this->listBoxGroups->SelectedIndexChanged += gcnew System::EventHandler(this, &WildcardForm::listBoxGroup_SelectedIndexChanged);
			// 
			// label1
			// 
			this->label1->AutoSize = true;
			this->label1->Location = System::Drawing::Point(13, 12);
			this->label1->Name = L"label1";
			this->label1->Size = System::Drawing::Size(81, 13);
			this->label1->TabIndex = 1;
			this->label1->Text = L"Wildcard Group";
			// 
			// textBoxWildcards
			// 
			this->textBoxWildcards->AcceptsReturn = true;
			this->textBoxWildcards->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom) 
				| System::Windows::Forms::AnchorStyles::Left) 
				| System::Windows::Forms::AnchorStyles::Right));
			this->textBoxWildcards->Location = System::Drawing::Point(164, 31);
			this->textBoxWildcards->Multiline = true;
			this->textBoxWildcards->Name = L"textBoxWildcards";
			this->textBoxWildcards->ScrollBars = System::Windows::Forms::ScrollBars::Both;
			this->textBoxWildcards->Size = System::Drawing::Size(290, 394);
			this->textBoxWildcards->TabIndex = 8;
			// 
			// label2
			// 
			this->label2->AutoSize = true;
			this->label2->Location = System::Drawing::Point(161, 12);
			this->label2->Name = L"label2";
			this->label2->Size = System::Drawing::Size(60, 13);
			this->label2->TabIndex = 3;
			this->label2->Text = L"Parameters";
			// 
			// buttonAdd
			// 
			this->buttonAdd->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Left));
			this->buttonAdd->Location = System::Drawing::Point(35, 253);
			this->buttonAdd->Name = L"buttonAdd";
			this->buttonAdd->Size = System::Drawing::Size(101, 23);
			this->buttonAdd->TabIndex = 1;
			this->buttonAdd->Text = L"Add &Group";
			this->buttonAdd->UseVisualStyleBackColor = true;
			this->buttonAdd->Click += gcnew System::EventHandler(this, &WildcardForm::buttonAdd_Click);
			// 
			// buttonRename
			// 
			this->buttonRename->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Left));
			this->buttonRename->Location = System::Drawing::Point(35, 309);
			this->buttonRename->Name = L"buttonRename";
			this->buttonRename->Size = System::Drawing::Size(101, 23);
			this->buttonRename->TabIndex = 3;
			this->buttonRename->Text = L"Re&name";
			this->buttonRename->UseVisualStyleBackColor = true;
			this->buttonRename->Click += gcnew System::EventHandler(this, &WildcardForm::buttonRename_Click);
			// 
			// buttonDelete
			// 
			this->buttonDelete->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Left));
			this->buttonDelete->Location = System::Drawing::Point(35, 337);
			this->buttonDelete->Name = L"buttonDelete";
			this->buttonDelete->Size = System::Drawing::Size(101, 23);
			this->buttonDelete->TabIndex = 4;
			this->buttonDelete->Text = L"&Delete";
			this->buttonDelete->UseVisualStyleBackColor = true;
			this->buttonDelete->Click += gcnew System::EventHandler(this, &WildcardForm::buttonDelete_Click);
			// 
			// buttonOK
			// 
			this->buttonOK->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Left));
			this->buttonOK->Location = System::Drawing::Point(16, 402);
			this->buttonOK->Name = L"buttonOK";
			this->buttonOK->Size = System::Drawing::Size(69, 23);
			this->buttonOK->TabIndex = 6;
			this->buttonOK->Text = L"OK";
			this->buttonOK->UseVisualStyleBackColor = true;
			this->buttonOK->Click += gcnew System::EventHandler(this, &WildcardForm::buttonOK_Click);
			// 
			// buttonCancel
			// 
			this->buttonCancel->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Left));
			this->buttonCancel->DialogResult = System::Windows::Forms::DialogResult::Cancel;
			this->buttonCancel->Location = System::Drawing::Point(89, 402);
			this->buttonCancel->Name = L"buttonCancel";
			this->buttonCancel->Size = System::Drawing::Size(69, 23);
			this->buttonCancel->TabIndex = 7;
			this->buttonCancel->Text = L"Cancel";
			this->buttonCancel->UseVisualStyleBackColor = true;
			// 
			// buttonHelp
			// 
			this->buttonHelp->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Left));
			this->buttonHelp->Location = System::Drawing::Point(35, 365);
			this->buttonHelp->Name = L"buttonHelp";
			this->buttonHelp->Size = System::Drawing::Size(101, 23);
			this->buttonHelp->TabIndex = 5;
			this->buttonHelp->Text = L"&Help";
			this->buttonHelp->UseVisualStyleBackColor = true;
			this->buttonHelp->Click += gcnew System::EventHandler(this, &WildcardForm::buttonHelp_Click);
			// 
			// buttonMove
			// 
			this->buttonMove->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Left));
			this->buttonMove->Location = System::Drawing::Point(35, 281);
			this->buttonMove->Name = L"buttonMove";
			this->buttonMove->Size = System::Drawing::Size(101, 23);
			this->buttonMove->TabIndex = 2;
			this->buttonMove->Text = L"&Move Up";
			this->buttonMove->UseVisualStyleBackColor = true;
			this->buttonMove->Click += gcnew System::EventHandler(this, &WildcardForm::buttonMove_Click);
			// 
			// WildcardForm
			// 
			this->AcceptButton = this->buttonOK;
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->CancelButton = this->buttonCancel;
			this->ClientSize = System::Drawing::Size(466, 437);
			this->Controls->Add(this->buttonMove);
			this->Controls->Add(this->buttonHelp);
			this->Controls->Add(this->buttonCancel);
			this->Controls->Add(this->buttonOK);
			this->Controls->Add(this->buttonDelete);
			this->Controls->Add(this->buttonRename);
			this->Controls->Add(this->buttonAdd);
			this->Controls->Add(this->label2);
			this->Controls->Add(this->textBoxWildcards);
			this->Controls->Add(this->label1);
			this->Controls->Add(this->listBoxGroups);
			this->Name = L"WildcardForm";
			this->ShowIcon = false;
			this->StartPosition = System::Windows::Forms::FormStartPosition::CenterParent;
			this->Text = L"Wildcard Replacement Parameters Editor";
			this->ResumeLayout(false);
			this->PerformLayout();

		}
#pragma endregion

private: System::Void listBoxGroup_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e) {
			 if( lastSelection >= 0 ) // update the wildcards of the previously selected group
				 wildcardParameters[lastSelection] = textBoxWildcards->Text;

			 bool validSelection = listBoxGroups->SelectedIndex >= 0;

			 buttonRename->Enabled = validSelection;
			 buttonDelete->Enabled = validSelection;
			 buttonMove->Enabled = validSelection && listBoxGroups->SelectedIndex > 0;
			 textBoxWildcards->ReadOnly = !validSelection;

			 if( validSelection )
				 textBoxWildcards->Text = (String^)wildcardParameters[listBoxGroups->SelectedIndex];

			 lastSelection = listBoxGroups->SelectedIndex;
		}

private: System::Void buttonHelp_Click(System::Object^  sender, System::EventArgs^  e) {
			listBoxGroups->ClearSelected();
			textBoxWildcards->ReadOnly = true;
			
			textBoxWildcards->Text = gcnew String(
				"In your PFF or DOS batch files you can place the string %%parameter%% anywhere and it will "
				"be replaced by the values that you list here.\r\n\r\n"
				"First create a group and then add a series of parameters to the group.\r\n\r\n"
				"A group's parameters can reference another group by surrounding the group name with %%. This "
				"only works, however, when the parameters do not result in recursive references.\r\n\r\n"
				"For example, in your PFF:\r\n\r\n"
				"InputData=..\\Data\\Raw\\%%parameter%%.dat\r\n"
				"OutputData=..\\Data\\Clean\\%%parameter%%.dat\r\n\r\n\r"
				"And in group DMV:\r\n\r\nDistrict of Columbia\r\nMaryland\r\nVirginia\r\n\r\n"
				"And in group West Coast:\r\n\r\nWashington\r\nOregon\r\nCalifornia\r\n\r\n"
				"And in group United States:\r\n\r\n%%DMV%%\r\n%%West Coast%%\r\n\r\n"
				"Now suppose that you run a PFF using all parameters from the group United States, the Production Runner will "
				"create six copies of the PFF, replacing the %%parameter%% text in the InputData and OutputData "
				"fields with the text District of Columbia, Maryland, ..., California.\r\n\r\n"
				"These temporary PFF files will be created in the same folder as the original PFF but will be "
				"erased after they have been executed.\r\n\r\n"
				"You can also use the string %%parameterRepeat%% to repeat the same line of text multiple times "
				"in the PFF, once for each parameter. This is especially useful for tabulation applications:\r\n\r\n"
				"InputData=..\\Data\\Clean\\%%parameterRepeat%%.dat\r\n\r\n"
				"This line in the PFF would allow you to create different tables based on what groups and "
				"parameters you select. Running the PFF using all parameters from the group West Coast would result in:\r\n\r\n"
				"InputData=..\\Data\\Clean\\Washington.dat\r\n"
				"InputData=..\\Data\\Clean\\Oregon.dat\r\n"
				"InputData=..\\Data\\Clean\\California.dat");
		}

private: System::Void buttonOK_Click(System::Object^  sender, System::EventArgs^  e) {
			listBoxGroups->ClearSelected(); // this ensures that the current text is added to wildcardParameters
			DialogResult = Windows::Forms::DialogResult::OK;
			Close();
		}

private: System::Void buttonAdd_Click(System::Object^  sender, System::EventArgs^  e) {
			 NameForm^ nf = gcnew NameForm("");

			 if( nf->ShowDialog() == Windows::Forms::DialogResult::OK )
			 {
				 for( int i = 0; i < wildcardGroups->Count; i++ )
				 {
					 if( !String::Compare((String^)wildcardGroups[i],nf->GroupName,true) )
					 {
						 MessageBox::Show("The group was not added because the name already exists!");
						 return;
					 }
				 }

				 wildcardGroups->Add(nf->GroupName);
				 wildcardParameters->Add("");
				 listBoxGroups->Items->Add(nf->GroupName);
				 listBoxGroups->SelectedIndex = listBoxGroups->Items->Count - 1;
			 }
		}

private: System::Void buttonRename_Click(System::Object^  sender, System::EventArgs^  e) {
			 NameForm^ nf = gcnew NameForm((String^)wildcardGroups[listBoxGroups->SelectedIndex]);

			 if( nf->ShowDialog() == Windows::Forms::DialogResult::OK )
			 {
				 for( int i = 0; i < wildcardGroups->Count; i++ )
				 {
					 if( !String::Compare((String^)wildcardGroups[i],nf->GroupName,true) )
					 {
						 if( i != listBoxGroups->SelectedIndex )
							 MessageBox::Show("The group name was not modified because the name already exists!");

						 return;
					 }
				 }
				 
				 int index = listBoxGroups->SelectedIndex;
				 wildcardGroups[index] = nf->GroupName;
				 listBoxGroups->Items->Insert(index + 1,nf->GroupName);
				 listBoxGroups->Items->RemoveAt(index);
				 listBoxGroups->SelectedIndex = index;
			 }
		 }

private: System::Void buttonDelete_Click(System::Object^  sender, System::EventArgs^  e) {
			 lastSelection = -1; // so we don't try to save this group's data
			 wildcardGroups->RemoveAt(listBoxGroups->SelectedIndex);
			 wildcardParameters->RemoveAt(listBoxGroups->SelectedIndex);

			 int nextSelectedIndex = listBoxGroups->SelectedIndex - 1;

			 if( nextSelectedIndex < 0 && listBoxGroups->Items->Count > 1 )
				 nextSelectedIndex = 0;

			 listBoxGroups->Items->RemoveAt(listBoxGroups->SelectedIndex);

			 listBoxGroups->SelectedIndex = nextSelectedIndex;
		}

private: System::Void buttonMove_Click(System::Object^  sender, System::EventArgs^  e) {
			 int index = listBoxGroups->SelectedIndex;
			 
			 wildcardParameters[index] = textBoxWildcards->Text;
			 lastSelection = -1; // so we don't try to save this group's data (because it is done above)
			 
			 Object^ savedGroup = wildcardGroups[index];
			 Object^ savedParameter = wildcardParameters[index];

			 wildcardGroups->RemoveAt(index);
			 wildcardGroups->Insert(index - 1,savedGroup);

			 wildcardParameters->RemoveAt(index);
			 wildcardParameters->Insert(index - 1,savedParameter);

			 listBoxGroups->Items->RemoveAt(listBoxGroups->SelectedIndex);
			 listBoxGroups->Items->Insert(index - 1,(String^)savedGroup);
			 
			 listBoxGroups->SelectedIndex = index - 1;			
		}
};
}
