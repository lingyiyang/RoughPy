// Copyright (c) 2023 Datasig Developers. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors
// may be used to endorse or promote products derived from this software without
// specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
// USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

//
// Created by user on 02/06/23.
//

#include "schema.h"


using namespace rpy;
using namespace rpy::python;
using namespace rpy::streams;
using namespace pybind11::literals;

namespace {


class PyChannelItem {
    StreamChannel* ptr;

public:

    PyChannelItem(StreamChannel& item) : ptr(&item)
    {}


};

inline void init_channel_item(py::module_& m) {

    py::enum_<ChannelType>(m, "ChannelType")
        .value("Increment", ChannelType::Increment)
        .value("Value", ChannelType::Value)
        .value("Categorical", ChannelType::Categorical)
        .export_values();

    py::class_<StreamChannel> cls(m, "StreamChannel");

}


}



void rpy::python::init_schema(py::module_ &m) {

    init_channel_item(m);

    py::class_<StreamSchema, std::shared_ptr<StreamSchema>>
        cls(m, "StreamSchema");


    cls.def_static("from_data", &parse_schema, "data"_a);

    cls.def("width", &StreamSchema::width);

    cls.def("insert_increment", [](StreamSchema* schema, string label) {
        return &schema->insert_increment(std::move(label));
    }, "label"_a, py::return_value_policy::reference_internal);

    cls.def("insert_value", [](StreamSchema* schema, string label) {
        return &schema->insert_value(std::move(label));
    }, "label"_a, py::return_value_policy::reference_internal);

    cls.def("insert_categorical", [](StreamSchema* schema, string label) {
        return &schema->insert_categorical(std::move(label));
    }, "label"_a, py::return_value_policy::reference_internal);

    cls.def("get_labels", [](const StreamSchema* schema) {
        py::list labels(schema->width());
        auto* plist = labels.ptr();
        py::ssize_t i = 0;
        for (auto&& item : *schema) {
            switch(item.second.type()) {
                case streams::ChannelType::Increment:
                    PyList_SET_ITEM(plist, i++, PyUnicode_FromString(item.first.c_str()));
                    break;
                case streams::ChannelType::Value:
                    PyList_SET_ITEM(plist, i++, PyUnicode_FromString((item.first + ':' + item.second.label_suffix(0)).c_str()));
                    PyList_SET_ITEM(plist, i++, PyUnicode_FromString((item.first + ':' + item.second.label_suffix(1)).c_str()));
                    break;
                case streams::ChannelType::Categorical:
                    auto nvariants = item.second.num_variants();
                    for (dimn_t idx=0; idx<nvariants; ++idx) {
                        PyList_SET_ITEM(plist, i++,
                                        PyUnicode_FromString((item.first + ':' + item.second.label_suffix(idx)).c_str()));
                    }
                    break;
            }
        }
        return labels;
    });

}


static std::shared_ptr<StreamSchema> parse_schema_from_dict_data(const py::dict& dict_data) {

    for (auto&& [timestamp, datum] : dict_data) {

    }


}
static std::shared_ptr<StreamSchema> parse_schema_from_seq_data(const py::sequence& sequence) {

}


std::shared_ptr<streams::StreamSchema> parse_schema(const py::object &data) {

    if (py::isinstance<py::dict>(data)) {
        return parse_schema_from_dict_data(py::reinterpret_steal<py::dict>(data));
    }

    if (py::isinstance<py::sequence>(data)) {
        return parse_schema_from_seq_data(py::reinterpret_steal<py::sequence>(data));
    }

    throw py::type_error("expected sequential data");
}