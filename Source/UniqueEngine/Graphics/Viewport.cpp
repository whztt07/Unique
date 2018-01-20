//
// Copyright (c) 2008-2017 the Unique project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "UniquePCH.h"
#include "Resource/ResourceCache.h"
#include "../Scene/Scene.h"
#include "../Graphics/Camera.h"
#include "../Graphics/Viewport.h"
#include "../Graphics/RenderPath.h"
#include "../Graphics/View.h"


#include "DebugNew.h"

namespace Unique
{
	uObject(Viewport)
	{
		uFactory("Renderer")
	}

Viewport::Viewport() : rect_(IntRect::ZERO), drawDebug_(true)
{
    SetRenderPath((RenderPath*)0);
}

Viewport::Viewport(Scene* scene, Camera* camera, RenderPath* renderPath) :
    scene_(scene),
    camera_(camera),
    rect_(IntRect::ZERO),
    drawDebug_(true)
{
    SetRenderPath(renderPath);
}

Viewport::Viewport(Scene* scene, Camera* camera, const IntRect& rect, RenderPath* renderPath) :
    scene_(scene),
    camera_(camera),
    rect_(rect),
    drawDebug_(true)
{
    SetRenderPath(renderPath);
}

Viewport::~Viewport()
{
}

void Viewport::SetScene(Scene* scene)
{
    scene_ = scene;
}

void Viewport::SetCamera(Camera* camera)
{
    camera_ = camera;
}

void Viewport::SetCullCamera(Camera* camera)
{
    cullCamera_ = camera;
}

void Viewport::SetRect(const IntRect& rect)
{
    rect_ = rect;
}

void Viewport::SetDrawDebug(bool enable)
{
    drawDebug_ = enable;
}

void Viewport::SetRenderPath(RenderPath* renderPath)
{
    if (renderPath)
        renderPath_ = renderPath;
    else
    {
        Renderer& renderer = GetSubsystem<Renderer>();
        renderPath_ = renderer.GetDefaultRenderPath();
    }
}

Scene* Viewport::GetScene() const
{
    return scene_;
}

Camera* Viewport::GetCamera() const
{
    return camera_;
}

Camera* Viewport::GetCullCamera() const
{
    return cullCamera_;
}

View* Viewport::GetView() const
{
    return view_;
}

RenderPath* Viewport::GetRenderPath() const
{
	return renderPath_;
}

Ray Viewport::GetScreenRay(int x, int y) const
{
    if (!camera_)
        return Ray();

    float screenX;
    float screenY;

    if (rect_ == IntRect::ZERO)
    {
        Graphics& graphics = GetSubsystem<Graphics>();
        screenX = (float)x / (float)graphics.GetResolution().x_;
        screenY = (float)y / (float)graphics.GetResolution().y_;
    }
    else
    {
        screenX = float(x - rect_.left_) / (float)rect_.Width();
        screenY = float(y - rect_.top_) / (float)rect_.Height();
    }

    return camera_->GetScreenRay(screenX, screenY);
}

IntVector2 Viewport::WorldToScreenPoint(const Vector3& worldPos) const
{
    if (!camera_)
        return IntVector2::ZERO;

    Vector2 screenPoint = camera_->WorldToScreenPoint(worldPos);

    int x;
    int y;
    if (rect_ == IntRect::ZERO)
    {
        /// \todo This is incorrect if the viewport is used on a texture rendertarget instead of the backbuffer, as it may have different dimensions.
        Graphics& graphics = GetSubsystem<Graphics>();
        x = (int)(screenPoint.x_ * graphics.GetResolution().x_);
        y = (int)(screenPoint.y_ * graphics.GetResolution().y_);
    }
    else
    {
        x = (int)(rect_.left_ + screenPoint.x_ * rect_.Width());
        y = (int)(rect_.top_ + screenPoint.y_ * rect_.Height());
    }

    return IntVector2(x, y);
}

Vector3 Viewport::ScreenToWorldPoint(int x, int y, float depth) const
{
    if (!camera_)
        return Vector3::ZERO;

    float screenX;
    float screenY;

    if (rect_ == IntRect::ZERO)
    {
        /// \todo This is incorrect if the viewport is used on a texture rendertarget instead of the backbuffer, as it may have different dimensions.
        Graphics& graphics = GetSubsystem<Graphics>();
        screenX = (float)x / (float)graphics.GetResolution().x_;
        screenY = (float)y / (float)graphics.GetResolution().y_;
    }
    else
    {
        screenX = float(x - rect_.left_) / (float)rect_.Width();
        screenY = float(y - rect_.top_) / (float)rect_.Height();
    }

    return camera_->ScreenToWorldPoint(Vector3(screenX, screenY, depth));
}

void Viewport::AllocateView()
{
    view_ = new View();
}

}