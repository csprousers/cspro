#pragma once

namespace CSProProductionRunner {

	public ref class PffTreeEntry {
	public:
		bool Group;
		bool TemporaryPFF;
		String^ OriginalPFFName;
		String^ WildcardParameter;
		PffTreeEntry(bool val) {
			Group = val;
			TemporaryPFF = false;
			OriginalPFFName = nullptr;
			WildcardParameter = nullptr;
		}
	};

}