#include "../include/demo.h"

#include "../include/demo_application.h"
#include "../include/link_constraint.h"

#include "../include/delta.h"

#include <cmath>

Demo::Demo() {
    m_app = nullptr;
    m_name = "";

    m_steps = 1;
    m_odeSolveMicroseconds = 0;
    m_forceEvalMicroseconds = 0;
    m_constraintEvalMicroseconds = 0;
    m_constraintSolveMicroseconds = 0;

    m_cursor_x = m_cursor_y = 0;
    m_activeBody = nullptr;
    m_targetSystem = nullptr;
    m_mouseObject = nullptr;

    m_interactionMode = InteractionMode::SelectDrag;
    m_awaitingSecondClick = false;
    m_firstClickObject = nullptr;
}

Demo::~Demo() {
    /* void */
}

void Demo::reset() {
    m_objects.clear();

    for (int i = 0; i < 16; ++i) {
        m_controlSprings[i] = nullptr;
    }

    m_activeBody = nullptr;
}

void Demo::setName(const std::string &name) {
    m_name = name;
}

void Demo::setApplication(DemoApplication *application) {
    m_app = application;
}

void Demo::initialize() {
    /* void */
}

void Demo::render() {
    // Render all demo objects
    renderObjects();
    
    // Render current interaction mode indicator
    const char* modeText = "";
    const char* modeHelp = "";
    
    switch (m_interactionMode) {
        case InteractionMode::SelectDrag: 
            modeText = "[1] DRAG MODE"; 
            modeHelp = "Click and drag objects";
            break;
        case InteractionMode::AddSpring: 
            modeText = "[2] ADD SPRING"; 
            modeHelp = m_awaitingSecondClick ? "Click second point..." : "Click two points to connect";
            break;
        case InteractionMode::AddFixedJoint: 
            modeText = "[4] FIX JOINT"; 
            modeHelp = "Click to lock position";
            break;
        case InteractionMode::AddRod: 
            modeText = "[5] ADD ROD"; 
            modeHelp = m_awaitingSecondClick ? "Click second point..." : "Click two points for rigid rod";
            break;
        case InteractionMode::Delete: 
            modeText = "[6] DELETE"; 
            modeHelp = "Click constraint to remove";
            break;
    }
    
    // Render mode text in top-right corner
    float gridWidth, gridHeight;
    m_app->getGridFrameSize(&gridWidth, &gridHeight);
    
    float textX = m_app->unitsToPixels(gridWidth/2) - 300;
    float textY = m_app->unitsToPixels(gridHeight/2) - 50;
    
    m_app->getTextRenderer()->RenderText(modeText, textX, textY, 20.0f);
    m_app->getTextRenderer()->RenderText(modeHelp, textX, textY + 25, 14.0f);
    
    // If waiting for second click in AddSpring mode, draw preview line
    if (m_interactionMode == InteractionMode::AddSpring && m_awaitingSecondClick) {
        // Draw a line from first click to current mouse position
        int mouseX, mouseY;
        m_app->getEngine()->GetOsMousePos(&mouseX, &mouseY);
        
        const int w = m_app->getScreenWidth();
        const int h = m_app->getScreenHeight();
        const double currentX = m_app->pixelsToUnits(mouseX - (float)w / 2);
        const double currentY = m_app->pixelsToUnits(mouseY - (float)h / 2);
        
        // Draw preview line (you'll need to implement this using your geometry generator)
        // This gives visual feedback showing where the spring will connect
        // m_app->drawLine(m_firstClickX, m_firstClickY, currentX, currentY);
    }
}

void Demo::process(float dt) {
    processObjects(dt);
}

void Demo::setInteractionMode(InteractionMode mode) {
    m_interactionMode = mode;
    m_awaitingSecondClick = false;
    m_firstClickObject = nullptr;
    
    // Visual feedback - could render current mode on screen
}

void Demo::handleModeSelection() {
    // Keyboard shortcuts for mode selection
    if (m_app->getEngine()->KeyDownEvent(ysKey::Code::N1)) {
        setInteractionMode(InteractionMode::SelectDrag);
    }
    else if (m_app->getEngine()->KeyDownEvent(ysKey::Code::N2)) {
        setInteractionMode(InteractionMode::AddSpring);
    }
    else if (m_app->getEngine()->KeyDownEvent(ysKey::Code::N3)) {
        setInteractionMode(InteractionMode::AddFixedJoint);
    }
    else if (m_app->getEngine()->KeyDownEvent(ysKey::Code::N4)) {
        setInteractionMode(InteractionMode::AddRod);
    }
    else if (m_app->getEngine()->KeyDownEvent(ysKey::Code::N5)) {
        setInteractionMode(InteractionMode::Delete);
    }
}

DemoObject* Demo::findObjectAt(double px, double py) {
    DemoObject::ClickEvent clickEvent{};
    clickEvent.clicked = false;
    
    const int n = (int)m_objects.size();
    for (int i = n - 1; i >= 0; --i) {
        m_objects[i]->onClick(px, py, &clickEvent);
        if (clickEvent.clicked) {
            return m_objects[i];
        }
    }
    return nullptr;
}

void Demo::processInput() {
    if (m_mouseObject == nullptr) return;
    
    // Handle mode selection
    handleModeSelection();
    
    int x, y;
    m_app->getEngine()->GetOsMousePos(&x, &y);
    
    const int w = m_app->getScreenWidth();
    const int h = m_app->getScreenHeight();
    const double px = m_app->pixelsToUnits(x - (float)w / 2);
    const double py = m_app->pixelsToUnits(y - (float)h / 2);
    
    if (m_app->getEngine()->ProcessMouseButtonDown(ysMouse::Button::Left)) {
        switch (m_interactionMode) {
            case InteractionMode::SelectDrag:
                // Your existing drag code
                handleSelectDrag(px, py);
                break;
                
            case InteractionMode::AddSpring:
                handleAddSpring(px, py);
                break;

            case InteractionMode::AddFixedJoint:
                handleAddFixedJoint(px, py);
                break;
                
            case InteractionMode::AddRod:
                handleAddRod(px, py);
                break;
                
            case InteractionMode::Delete:
                handleDelete(px, py);
                break;
        }
    }
    
    // Update mouse object position
    m_mouseObject->m_body.p_x = px;
    m_mouseObject->m_body.p_y = py;
}

void Demo::handleSelectDrag(double px, double py) {
    // Your existing code - extract from processInput()
    DemoObject *clickedObject = findObjectAt(px, py);
    
    if (clickedObject != nullptr) {
        DemoObject::ClickEvent clickEvent{};
        clickedObject->onClick(px, py, &clickEvent);
        
        for (int i = 0; i < 16; ++i) {
            if (m_controlSprings[i] == nullptr) continue;
            
            if (m_controlSprings[i]->getSystem() == clickedObject->getSystem()) {
                m_controlSprings[i]->m_spring.m_body1 = &m_mouseObject->m_body;
                m_controlSprings[i]->m_spring.m_body2 = clickEvent.body;
                m_controlSprings[i]->setVisible(true);
                m_controlSprings[i]->m_coilCount = 8;
                m_controlSprings[i]->m_spring.m_restLength = 0;
                
                double lx, ly;
                clickEvent.body->worldToLocal(clickEvent.x, clickEvent.y, &lx, &ly);
                m_controlSprings[i]->m_spring.m_p1_x = 0;
                m_controlSprings[i]->m_spring.m_p1_y = 0;
                m_controlSprings[i]->m_spring.m_p2_x = lx;
                m_controlSprings[i]->m_spring.m_p2_y = ly;
            }
        }
    }
    else {
        for (int i = 0; i < 16; ++i) {
            if (m_controlSprings[i] == nullptr) continue;
            m_controlSprings[i]->setVisible(false);
            m_controlSprings[i]->m_spring.m_body1 = nullptr;
            m_controlSprings[i]->m_spring.m_body2 = nullptr;
        }
    }
}

void Demo::handleAddSpring(double px, double py) {
    DemoObject* clickedObject = findObjectAt(px, py);
    
    if (!m_awaitingSecondClick) {
        // First click - store it
        if (clickedObject != nullptr) {
            DemoObject::ClickEvent clickEvent{};
            clickedObject->onClick(px, py, &clickEvent);
            
            if (clickEvent.clicked) {
                m_firstClickObject = clickedObject;
                m_firstClickX = px;
                m_firstClickY = py;
                m_awaitingSecondClick = true;
            }
        }
    }
    else {
        // Second click - create spring
        if (clickedObject != nullptr) {
            DemoObject::ClickEvent event1{}, event2{};
            m_firstClickObject->onClick(m_firstClickX, m_firstClickY, &event1);
            clickedObject->onClick(px, py, &event2);
            
            if (event1.clicked && event2.clicked) {
                double lx1, ly1, lx2, ly2;
                event1.body->worldToLocal(event1.x, event1.y, &lx1, &ly1);
                event2.body->worldToLocal(event2.x, event2.y, &lx2, &ly2);
                
                // NOW CREATE THE SPRING WITH THE BODY REFERENCES
                createUserSpringWithBodies(
                    event1.body, lx1, ly1,
                    event2.body, lx2, ly2,
                    m_firstClickObject->getSystem()
                );
            }
        }
        
        m_awaitingSecondClick = false;
        m_firstClickObject = nullptr;
    }
}

void Demo::createUserSpringWithBodies(atg_scs::RigidBody* body1, double lx1, double ly1,
                                     atg_scs::RigidBody* body2, double lx2, double ly2,
                                     atg_scs::RigidBodySystem* system) {
    // Calculate rest length
    double wx1, wy1, wx2, wy2;
    body1->localToWorld(lx1, ly1, &wx1, &wy1);
    body2->localToWorld(lx2, ly2, &wx2, &wy2);
    double dx = wx2 - wx1;
    double dy = wy2 - wy1;
    double restLength = std::sqrt(dx*dx + dy*dy);
    
    // Create visual spring using existing template (already adds to system)
    SpringObject* springObj = new SpringObject();
    springObj->m_spring.m_body1 = body1;
    springObj->m_spring.m_body2 = body2;
    springObj->m_spring.m_p1_x = lx1;
    springObj->m_spring.m_p1_y = ly1;
    springObj->m_spring.m_p2_x = lx2;
    springObj->m_spring.m_p2_y = ly2;
    springObj->m_spring.m_restLength = restLength;
    springObj->m_spring.m_ks = 100.0;
    springObj->m_spring.m_kd = 0.0;
    springObj->m_coilCount = 8;
    springObj->setVisible(true);
    
    addObject(springObj, system);
    // Store constraint data
    ConstraintData constraint;
    constraint.type = ConstraintData::Spring;
    constraint.body1 = body1;
    constraint.body2 = body2;
    constraint.localX1 = lx1;
    constraint.localY1 = ly1;
    constraint.localX2 = lx2;
    constraint.localY2 = ly2;
    constraint.springK = 500.0;
    constraint.springDamping = 10.0;
    constraint.restLength = restLength;
    constraint.visualObject = springObj;
    
    m_userConstraints.push_back(constraint);
}

// Keep the old signature but make it call the new one (for compatibility)
void Demo::createUserSpring(DemoObject* obj1, double lx1, double ly1,
                            DemoObject* obj2, double lx2, double ly2) {
    // This is a compatibility wrapper - not used anymore
}
void Demo::handleAddRod(double px, double py) {
    DemoObject* clickedObject = findObjectAt(px, py);
    
    if (!m_awaitingSecondClick) {
        // First click - store it
        if (clickedObject != nullptr) {
            DemoObject::ClickEvent clickEvent{};
            clickedObject->onClick(px, py, &clickEvent);
            
            if (clickEvent.clicked) {
                m_firstClickObject = clickedObject;
                m_firstClickX = px;
                m_firstClickY = py;
                m_awaitingSecondClick = true;
            }
        }
    }
    else {
        // Second click - create rod
        if (clickedObject != nullptr) {
            DemoObject::ClickEvent event1{}, event2{};
            m_firstClickObject->onClick(m_firstClickX, m_firstClickY, &event1);
            clickedObject->onClick(px, py, &event2);
            
            if (event1.clicked && event2.clicked) {
                double lx1, ly1, lx2, ly2;
                event1.body->worldToLocal(event1.x, event1.y, &lx1, &ly1);
                event2.body->worldToLocal(event2.x, event2.y, &lx2, &ly2);
                
                createUserRodWithBodies(
                    event1.body, lx1, ly1,
                    event2.body, lx2, ly2,
                    m_firstClickObject->getSystem()
                );
            }
        }
        
        m_awaitingSecondClick = false;
        m_firstClickObject = nullptr;
    }
}

void Demo::createUserRodWithBodies(atg_scs::RigidBody* body1, double lx1, double ly1,
                                  atg_scs::RigidBody* body2, double lx2, double ly2,
                                  atg_scs::RigidBodySystem* system) {
    // VALIDATION
    if (body1 == body2) return;
    
    // Calculate world positions
    double wx1, wy1, wx2, wy2;
    body1->localToWorld(lx1, ly1, &wx1, &wy1);
    body2->localToWorld(lx2, ly2, &wx2, &wy2);
    
    double dx = wx2 - wx1;
    double dy = wy2 - wy1;
    double rodLength = std::sqrt(dx*dx + dy*dy);
    
    if (rodLength < 0.01) return;
    if (body1->m == 0.0 && body2->m == 0.0) return;
    
    // Calculate position and angle
    double theta = std::atan2(dy, dx);
    double midX = (wx1 + wx2) / 2.0;
    double midY = (wy1 + wy2) / 2.0;
    
    // Create the bar (rigid rod body)
    BarObject* barObj = new BarObject();
    barObj->m_body.p_x = midX;
    barObj->m_body.p_y = midY;
    barObj->m_body.theta = theta;
    barObj->m_body.v_x = 0;
    barObj->m_body.v_y = 0;
    barObj->m_body.v_theta = 0;
    barObj->configure(rodLength, 0.001);  // Length and density
    
    // Add bar to system
    addObject(barObj, system);
    
    // Connect left end of bar to body1
    LinkConstraint* link1 = new LinkConstraint();
    link1->m_link.setBody1(body1);
    link1->m_link.setBody2(&barObj->m_body);
    link1->m_link.setLocalPosition1(lx1, ly1);
    link1->m_link.setLocalPosition2(-rodLength/2.0, 0.0);  // Left end
    
    // Optional: Make the link stiffer (more rigid)
    link1->m_link.m_ks = 500.0;  // Higher stiffness = more rigid
    link1->m_link.m_kd = 10.0;   // Damping
    
    addObject(link1, system);
    
    // Connect right end of bar to body2
    LinkConstraint* link2 = new LinkConstraint();
    link2->m_link.setBody1(body2);
    link2->m_link.setBody2(&barObj->m_body);
    link2->m_link.setLocalPosition1(lx2, ly2);
    link2->m_link.setLocalPosition2(rodLength/2.0, 0.0);   // Right end
    
    // Match stiffness
    link2->m_link.m_ks = 500.0;
    link2->m_link.m_kd = 10.0;
    
    addObject(link2, system);
    
    // Store constraint data
    ConstraintData constraint;
    constraint.type = ConstraintData::Rod;
    constraint.body1 = body1;
    constraint.body2 = body2;
    constraint.localX1 = lx1;
    constraint.localY1 = ly1;
    constraint.localX2 = lx2;
    constraint.localY2 = ly2;
    constraint.rodLength = rodLength;
    constraint.visualObject = barObj;
    
    m_userConstraints.push_back(constraint);
}



void Demo::handleAddFixedJoint(double px, double py) {
    DemoObject* clickedObject = findObjectAt(px, py);
    
    if (clickedObject != nullptr) {
        DemoObject::ClickEvent clickEvent{};
        clickedObject->onClick(px, py, &clickEvent);
        
        if (clickEvent.clicked) {
            // Create a fixed position constraint
            ConstraintData constraint;
            constraint.type = ConstraintData::FixedJoint;
            constraint.body1 = clickEvent.body;
            constraint.body2 = nullptr;
            constraint.localX1 = 0;
            constraint.localY1 = 0;
            
            // Add to system as fixed constraint
            // ... implementation depends on your constraint system ...
            
            m_userConstraints.push_back(constraint);
        }
    }
}

void Demo::handleDelete(double px, double py) {
    for (int i = (int)m_userConstraints.size() - 1; i >= 0; --i) {
        ConstraintData& constraint = m_userConstraints[i];
        
        // --- Proximity check ---
        bool shouldDelete = false;

        if (constraint.type == ConstraintData::Spring && constraint.visualObject != nullptr) {
            // Check proximity to the spring's midpoint in world space
            double wx1, wy1, wx2, wy2;
            constraint.body1->localToWorld(constraint.localX1, constraint.localY1, &wx1, &wy1);
            constraint.body2->localToWorld(constraint.localX2, constraint.localY2, &wx2, &wy2);
            double midX = (wx1 + wx2) / 2.0;
            double midY = (wy1 + wy2) / 2.0;
            double dx = px - midX, dy = py - midY;
            if (std::sqrt(dx*dx + dy*dy) < 0.5) shouldDelete = true;
        }
        else if (constraint.type == ConstraintData::Rod && constraint.visualObject != nullptr) {
            BarObject* bar = static_cast<BarObject*>(constraint.visualObject);
            double dx = px - bar->m_body.p_x;
            double dy = py - bar->m_body.p_y;
            if (std::sqrt(dx*dx + dy*dy) < 0.5) shouldDelete = true;
        }

        if (!shouldDelete) continue;

        // --- Remove all associated DemoObjects from m_objects and delete them ---
        // For a rod, the visualObject is the BarObject, but we also need to remove
        // the two LinkConstraints that were added immediately after it in m_objects.
        // We collect them by scanning m_objects for objects added after the visual one.

        auto removeFromObjects = [&](DemoObject* obj) {
            if (obj == nullptr) return;
            obj->deinitialize(); // remove from physics system
            auto it = std::find(m_objects.begin(), m_objects.end(), obj);
            if (it != m_objects.end()) {
                m_objects.erase(it);
            }
            delete obj;
        };

        if (constraint.type == ConstraintData::Rod) {
            // The rod was added as: barObj, link1, link2 consecutively.
            // Find the bar's index and grab the two LinkConstraints after it.
            auto it = std::find(m_objects.begin(), m_objects.end(), constraint.visualObject);
            if (it != m_objects.end()) {
                // Collect the bar + the two links after it before erasing
                DemoObject* barObj  = *it;
                DemoObject* link1   = (it + 1 != m_objects.end()) ? *(it + 1) : nullptr;
                DemoObject* link2   = (it + 2 != m_objects.end()) ? *(it + 2) : nullptr;

                // Deinitialize and erase in reverse order to keep iterators valid
                auto eraseAndDelete = [&](DemoObject* obj) {
                    if (!obj) return;
                    obj->deinitialize();
                    auto jt = std::find(m_objects.begin(), m_objects.end(), obj);
                    if (jt != m_objects.end()) m_objects.erase(jt);
                    delete obj;
                };

                eraseAndDelete(link2);
                eraseAndDelete(link1);
                eraseAndDelete(barObj);
            }
        }
        else {
            removeFromObjects(constraint.visualObject);
        }

        m_userConstraints.erase(m_userConstraints.begin() + i);
        break; // Only delete one constraint per click
    }
}

double Demo::energy(atg_scs::RigidBodySystem *system) {
    double energy = 0;
    for (DemoObject *object : m_objects) {
        if (object->getSystem() == system || system == nullptr) {
            energy += object->energy();
        }
    }

    return energy;
}

void Demo::clear() {
    for (DemoObject *obj : m_objects) {
        delete obj;
    }

    m_objects.clear();

    m_activeBody = nullptr;
    m_targetSystem = nullptr;
    m_cursor_x = m_cursor_y = 0;

    for (int i = 0; i < 16; ++i) {
        m_controlSprings[i] = nullptr;
    }
}

void Demo::addObject(DemoObject *object, atg_scs::RigidBodySystem *system) {
    m_objects.push_back(object);
    object->initialize(system);
}

void Demo::processObjects(float dt) {
    for (DemoObject *object : m_objects) {
        object->process(dt, m_app);
    }
}

void Demo::renderObjects() {
    for (DemoObject *object : m_objects) {
        if (object->isVisible()) {
            object->render(m_app);
        }
    }
}

void Demo::setCursor(double x, double y) {
    m_cursor_x = x;
    m_cursor_y = y;
}

void Demo::moveCursor(double x, double y) {
    m_cursor_x += x;
    m_cursor_y += y;
}

void Demo::setTargetSystem(atg_scs::RigidBodySystem *system) {
    m_targetSystem = system;
}

void Demo::setActiveBody(atg_scs::RigidBody *body) {
    m_activeBody = body;
}

void Demo::moveToLocal(double lx, double ly) {
    double x, y;
    m_activeBody->localToWorld(lx, ly, &x, &y);

    m_cursor_x = x;
    m_cursor_y = y;
}

FixedPositionConstraint *Demo::fixObject(double x, double y) {
    double l_x, l_y;
    m_activeBody->worldToLocal(x, y, &l_x, &l_y);

    FixedPositionConstraint *newConstraint =
        createObject<FixedPositionConstraint>(m_targetSystem);

    newConstraint->m_link.setBody(m_activeBody);
    newConstraint->m_link.setLocalPosition(l_x, l_y);
    newConstraint->m_link.setWorldPosition(x, y);

    return newConstraint;
}

LinkConstraint *Demo::connectObjects(atg_scs::RigidBody *target) {
    double x0, y0, x1, y1;
    m_activeBody->worldToLocal(m_cursor_x, m_cursor_y, &x0, &y0);
    target->worldToLocal(m_cursor_x, m_cursor_y, &x1, &y1);

    LinkConstraint *link = createObject<LinkConstraint>(m_targetSystem);
    link->m_link.setBody2(m_activeBody);
    link->m_link.setBody1(target);
    link->m_link.setLocalPosition2(x0, y0);
    link->m_link.setLocalPosition1(x1, y1);

    return link;
}

BarObject *Demo::createLinkedBar(double x, double y, double density) {
    const double dx = x - m_cursor_x;
    const double dy = y - m_cursor_y;
    const double length = std::sqrt(dx * dx + dy * dy);

    const double theta = (dy > 0)
        ? std::acos(dx / length)
        : ysMath::Constants::TWO_PI - std::acos(dx / length);

    BarObject *newBar = createObject<BarObject>(m_targetSystem);
    newBar->m_body.theta = theta;

    double ex, ey;
    newBar->m_body.localToWorld(-length / 2, 0, &ex, &ey);

    newBar->m_body.p_x = m_cursor_x - ex;
    newBar->m_body.p_y = m_cursor_y - ey;

    newBar->configure(length, density);

    if (m_activeBody != nullptr) {
        double x0, y0;
        m_activeBody->worldToLocal(m_cursor_x, m_cursor_y, &x0, &y0);

        LinkConstraint *link = createObject<LinkConstraint>(m_targetSystem);
        link->m_link.setBody2(m_activeBody);
        link->m_link.setBody1(&newBar->m_body);
        link->m_link.setLocalPosition2(x0, y0);
        link->m_link.setLocalPosition1(-length / 2, 0);
    }

    double new_x, new_y;
    newBar->m_body.localToWorld(length / 2, 0, &new_x, &new_y);
    m_cursor_x = new_x;
    m_cursor_y = new_y;

    m_activeBody = &newBar->m_body;

    return newBar;
}

EmptyObject *Demo::createEmpty(EmptyObject::Style style) {
    EmptyObject *newEmpty =
        createObject<EmptyObject>(m_targetSystem);
    newEmpty->m_body.p_x = m_cursor_x;
    newEmpty->m_body.p_y = m_cursor_y;
    newEmpty->m_style = style;

    m_activeBody = &newEmpty->m_body;

    return newEmpty;
}

DiskObject *Demo::createLinkedDisk(double r, double density) {
    DiskObject *newDisk =
        createObject<DiskObject>(m_targetSystem);
    newDisk->configure(r, density);
    newDisk->m_body.p_x = m_cursor_x;
    newDisk->m_body.p_y = m_cursor_y;
    newDisk->m_body.theta = 0;

    if (m_activeBody != nullptr) {
        double x0, y0;
        m_activeBody->worldToLocal(m_cursor_x, m_cursor_y, &x0, &y0);

        LinkConstraint *link = createObject<LinkConstraint>(m_targetSystem);
        link->m_link.setBody1(m_activeBody);
        link->m_link.setBody2(&newDisk->m_body);
        link->m_link.setLocalPosition1(x0, y0);
        link->m_link.setLocalPosition2(0, 0);
    }

    m_activeBody = &newDisk->m_body;

    return newDisk;
}

SpringObject *Demo::connectSpring(atg_scs::RigidBody *target, double x, double y) {
    SpringObject *newSpring = createObject<SpringObject>(m_targetSystem);
    newSpring->m_spring.m_body1 = m_activeBody;
    newSpring->m_spring.m_body2 = target;

    double lx0, ly0, lx1, ly1;
    newSpring->m_spring.m_body1->worldToLocal(m_cursor_x, m_cursor_y, &lx0, &ly0);
    newSpring->m_spring.m_body2->worldToLocal(x, y, &lx1, &ly1);

    const double dx = x - m_cursor_x;
    const double dy = y - m_cursor_y;

    newSpring->m_spring.m_p1_x = lx0;
    newSpring->m_spring.m_p1_y = ly0;
    newSpring->m_spring.m_p2_x = lx1;
    newSpring->m_spring.m_p2_y = ly1;
    newSpring->m_spring.m_restLength = std::sqrt(dx * dx + dy * dy);

    return newSpring;
}

ConstantSpeedMotor *Demo::createMotor(atg_scs::RigidBody *base) {
    ConstantSpeedMotor *newMotor = createObject<ConstantSpeedMotor>(m_targetSystem);
    newMotor->m_motor.m_body0 = base;
    newMotor->m_motor.m_body1 = m_activeBody;

    double lx, ly;
    base->worldToLocal(m_cursor_x, m_cursor_y, &lx, &ly);

    newMotor->m_local_x = (float)lx;
    newMotor->m_local_y = (float)ly;

    return newMotor;
}

SpringObject *Demo::createControlSpring(double ks, double kd) {
    SpringObject **target = nullptr;
    for (int i = 0; i < 16; ++i) {
        if (m_controlSprings[i] == nullptr) {
            target = &m_controlSprings[i];
            break;
        }
    }

    if (target == nullptr) return nullptr;

    SpringObject *newSpring = createObject<SpringObject>(m_targetSystem);
    newSpring->m_spring.m_body1 = nullptr;
    newSpring->m_spring.m_body2 = nullptr;
    newSpring->m_spring.m_ks = ks;
    newSpring->m_spring.m_kd = kd;
    newSpring->setVisible(false);
    *target = newSpring;

    return newSpring;
}

EmptyObject *Demo::createMouseEmpty(EmptyObject::Style style) {
    return m_mouseObject = createEmpty(style);
}

void Demo::moveBefore(DemoObject *a, DemoObject *b) {
    const size_t n = m_objects.size();
    if (n == 0) return;

    size_t i_a = 0, i_b = 0;
    for (size_t i = 0; i < n; ++i) {
        if (m_objects[i] == a) {
            i_a = i;
        }
        else if (m_objects[i] == b) {
            i_b = i;
        }
    }

    if (i_a <= i_b) return;
    else if (m_objects[i_a] != a) return;
    else if (m_objects[i_b] != b) return;

    m_objects[i_b] = a;

    DemoObject *prev = m_objects[i_b + 1];
    m_objects[i_b + 1] = b;

    for (size_t i = i_b + 2; i <= i_a; ++i) {
        DemoObject *newPrev = m_objects[i];
        m_objects[i] = prev;
        prev = newPrev;
    }
}
