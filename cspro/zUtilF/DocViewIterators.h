#pragma once


// --------------------------------------------------------------------------
// With... functions
// --------------------------------------------------------------------------

template<typename ViewType = CView, typename CF>
void WithFirstView(CDocument& doc, const CF& callback_function);

template<typename CF>
void WithParentFrame(CDocument& doc, const CF& callback_function);


// --------------------------------------------------------------------------
// Foreach... functions
// the callback functions should return true to keep processing
// --------------------------------------------------------------------------

template<typename DocType = CDocument, typename CF>
void ForeachDoc(const CF& callback_function);

template<typename DocType, typename CF>
void ForeachDocOfType(const CF& callback_function);

template<typename ViewType = CView, typename CF>
void ForeachView(CDocument& doc, const CF& callback_function);

template<typename ViewType, typename CF>
void ForeachViewOfType(CDocument& doc, const CF& callback_function);

template<typename ViewType = CView, typename CF>
void ForeachView(const CF& callback_function);

template<typename ViewType, typename CF>
void ForeachViewOfType(const CF& callback_function);

template<typename CF>
void ForeachParentFrame(const CF& callback_function);



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

template<typename ViewType/* = CView*/, typename CF>
void WithFirstView(CDocument& doc, const CF& callback_function)
{
    POSITION view_pos = doc.GetFirstViewPosition();

    if( view_pos != nullptr )
        callback_function(*assert_cast<ViewType*>(doc.GetNextView(view_pos)));
}


template<typename CF>
void WithParentFrame(CDocument& doc, const CF& callback_function)
{
    POSITION view_pos = doc.GetFirstViewPosition();

    if( view_pos == nullptr )
        return;

    CView* const view = doc.GetNextView(view_pos);

    if( view == nullptr )
        return;

    CFrameWnd* const frame_wnd = view->GetParentFrame();

    if( frame_wnd != nullptr )
        callback_function(*frame_wnd);
}


template<typename DocType/* = CDocument*/, typename CF>
void ForeachDoc(const CF& callback_function)
{
    POSITION template_pos = AfxGetApp()->GetFirstDocTemplatePosition();

    while( template_pos != nullptr )
    {
        CDocTemplate* doc_template = AfxGetApp()->GetNextDocTemplate(template_pos);
        POSITION doc_pos = doc_template->GetFirstDocPosition();

        while( doc_pos != nullptr )
        {
            if( !callback_function(*assert_cast<DocType*>(doc_template->GetNextDoc(doc_pos))) )
                return;
        }
    }
}


template<typename DocType, typename CF>
void ForeachDocOfType(const CF& callback_function)
{
    POSITION template_pos = AfxGetApp()->GetFirstDocTemplatePosition();

    while( template_pos != nullptr )
    {
        CDocTemplate* const doc_template = AfxGetApp()->GetNextDocTemplate(template_pos);
        POSITION doc_pos = doc_template->GetFirstDocPosition();

        while( doc_pos != nullptr )
        {
            CDocument* const doc = doc_template->GetNextDoc(doc_pos);

            if( doc->IsKindOf(RUNTIME_CLASS(DocType)) )
            {
                if( !callback_function(*assert_cast<DocType*>(doc)) )
                    return;
            }
        }
    }
}


template<typename ViewType/* = CView*/, typename CF>
void ForeachView(CDocument& doc, const CF& callback_function)
{
    POSITION view_pos = doc.GetFirstViewPosition();

    while( view_pos != nullptr )
    {
        if( !callback_function(*assert_cast<ViewType*>(doc.GetNextView(view_pos))) )
            return;
    }
}


template<typename ViewType, typename CF>
void ForeachViewOfType(CDocument& doc, const CF& callback_function)
{
    POSITION view_pos = doc.GetFirstViewPosition();

    while( view_pos != nullptr )
    {
        ViewType* view_of_type = dynamic_cast<ViewType*>(doc.GetNextView(view_pos));

        if( view_of_type != nullptr && !callback_function(*view_of_type) )
            return;
    }
}


template<typename ViewType/* = CView*/, typename CF>
void ForeachView(const CF& callback_function)
{
    ForeachDoc(
        [&](CDocument& doc)
        {
            POSITION view_pos = doc.GetFirstViewPosition();

            while( view_pos != nullptr )
            {
                if( !callback_function(*assert_cast<ViewType*>(doc.GetNextView(view_pos))) )
                    return false;
            }

            return true;
        });
}


template<typename ViewType, typename CF>
void ForeachViewOfType(const CF& callback_function)
{
    ForeachDoc(
        [&](CDocument& doc)
        {
            POSITION view_pos = doc.GetFirstViewPosition();

            while( view_pos != nullptr )
            {
                ViewType* view_of_type = dynamic_cast<ViewType*>(doc.GetNextView(view_pos));

                if( view_of_type != nullptr && !callback_function(*view_of_type) )
                    return false;
            }

            return true;
        });
}


template<typename CF>
void ForeachParentFrame(const CF& callback_function)
{
    ForeachDoc(
        [&](CDocument& doc)
        {
            POSITION view_pos = doc.GetFirstViewPosition();

            if( view_pos != nullptr )
            {
                CView* const view = doc.GetNextView(view_pos);
                CFrameWnd* const frame_wnd = ( view != nullptr ) ? view->GetParentFrame() :
                                                                   nullptr;

                if( frame_wnd != nullptr && !callback_function(*frame_wnd) )
                    return false;
            }

            return true;
        });
}
