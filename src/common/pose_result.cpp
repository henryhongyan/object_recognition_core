/*
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2009, Willow Garage, Inc.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Willow Garage, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <map>

#include <object_recognition_core/common/pose_result.h>

namespace object_recognition_core
{
  namespace common
  {
    /** Define the static member */
    std::map<std::string, PoseResult::Attributes> PoseResult::cached_name_mesh_id_ =
        std::map<std::string, Attributes>();

    /** Read the name_ and mesh_id_ from the DB and store it */
    void
    PoseResult::check_db() const
    {
      if (is_db_checked_)
        return;
      is_db_checked_ = true;

      // Check if the data is already cached
      {
        std::map<std::string, Attributes>::const_iterator iter = cached_name_mesh_id_.find(cache_key());
        if (iter != cached_name_mesh_id_.end())
        {
          attributes_ = iter->second;
          return;
        }
      }

      // Get the object name if not set
      if (attributes_.fields_.find("name") == attributes_.fields_.end())
      {
        db::Document doc(db_, object_id_);
        attributes_.fields_["name"] = doc.get_value("object_name").get_str();

        // If no name, set one
        if (attributes_.fields_["name"].get_str().empty())
          attributes_.fields_["name"] = object_id_;
      }

      // Find all the models of that type for that object
      db::View view(db::View::VIEW_MODEL_WHERE_OBJECT_ID_AND_MODEL_TYPE);
      view.Initialize("mesh");
      view.set_key(object_id_);

      // Make sure the db_ is valid
      if (db_.parameters().type_ == db::ObjectDbParameters::EMPTY)
        throw std::runtime_error("Db not set in the PoseResult");

      // Get the mesh id
      db::ViewIterator view_iterator(view, db_);

      db::ViewIterator iter = view_iterator.begin(), end = view_iterator.end();
      std::string mesh_id;
      for (; iter != end; ++iter)
      {
        // Get the mesh_id if not set
        if (attributes_.fields_.find("mesh_id") == attributes_.fields_.end())
          mesh_id = (*iter).value_.get_obj().find("_id")->second.get_str();

        // The view should return only one element
        break;
      }

      // Get the mesh_URI if not set
      if (attributes_.fields_.find("mesh_uri") == attributes_.fields_.end())
      {
        switch (db_.parameters().type_)
        {
          case db::ObjectDbParameters::COUCHDB:
            // E.g. http://localhost:5984/object_recognition/_design/models/_view/by_object_id_and_mesh?key=%2212a1e6eb663a41f8a4fb9baa060f191c%22
            if (!mesh_id.empty())
              attributes_.fields_["mesh_uri"] = db_.parameters().root_ + std::string("/") + db_.parameters().collection_
                                                + "/" + mesh_id + "/mesh.stl";
            break;
          default:
            attributes_.fields_["mesh_uri"] = "";
            break;
        }
      }

      // Cache all the results
      cached_name_mesh_id_[cache_key()] = attributes_;
    }
  }
}
