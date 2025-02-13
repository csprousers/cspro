#include "StdAfx.h"
#include "RunData.h"


namespace CSProProductionRunner {

	bool isConcurrent(TreeNode^ tn) {
			 return tn->Text->EndsWith(CONCURRENT_TEXT);
		 }
}
