#include "datalist.h"
////////////////////////////////////////////////////////////////////////////////
namespace Core {
////////////////////////////////////////////////////////////////////////////////
    DataList::DataList(){
    }
////////////////////////////////////////////////////////////////////////////////
    DataList::~DataList(){
    }
////////////////////////////////////////////////////////////////////////////////
    void
    DataList::AddData(Data *data){
        data_array.push_back(data);
    }
///////////////////////////////////////////////////////////////////////////////
     int
     DataList::Size(){
        return data_array.size();
     }
////////////////////////////////////////////////////////////////////////////////
} // namespace Core
