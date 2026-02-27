#ifndef ATG_SCS_2D_DEMO_DEMO_H
#define ATG_SCS_2D_DEMO_DEMO_H

#include "demo_objects.h"

#include <string>
#include <vector>

class DemoApplication;

enum class InteractionMode {
    None,           // No interaction
    SelectDrag,     // Current behavior - drag objects with spring
    AddSpring,      // Click two points to add spring
    AddFixedJoint,  // Click point to fix in place
    AddRod,
    Delete          // Click to delete constraint
};

struct ConstraintData {
    enum Type {
        Spring,
        FixedJoint,
        Rod
    };
    
    Type type;
    atg_scs::RigidBody* body1;
    atg_scs::RigidBody* body2;
    double localX1, localY1;
    double localX2, localY2;
    double springK;
    double springDamping;
    double restLength;
    double rodLength;
    DemoObject* visualObject;
};

class Demo {
    public:
        Demo();
        virtual ~Demo();

        void reset();
        void setName(const std::string &name);
        void setApplication(DemoApplication *application);

        virtual void initialize();
        virtual void render(); 
        virtual void process(float dt);
        void processInput();
        double energy(atg_scs::RigidBodySystem *system = nullptr);

        std::string getName() const { return m_name; }

        int getSteps() const { return m_steps; }
        float getTimestep() const { return m_dt; }
        float getOdeSolveMicroseconds() const { return m_odeSolveMicroseconds; }
        float getForceEvalMicroseconds() const { return m_forceEvalMicroseconds; }
        float getConstraintEvalMicroseconds() const { return m_constraintEvalMicroseconds; }
        float getConstraintSolveMicroseconds() const { return m_constraintSolveMicroseconds; }

        // Interaction mode - public so toolbar can read/set it
        void setInteractionMode(InteractionMode mode);
        InteractionMode getInteractionMode() const { return m_interactionMode; }

        template <typename T>
        T *createObject(atg_scs::RigidBodySystem *system) {
            T *newObject = new T;
            addObject(newObject, system);
            return newObject;
        }
        void clear();

    protected:
        void addObject(DemoObject *object, atg_scs::RigidBodySystem *system);

        void renderObjects();
        void processObjects(float dt);

        DemoApplication *m_app;
        std::string m_name;

        std::vector<DemoObject *> m_objects;

        int m_steps;
        float m_dt;
        float m_odeSolveMicroseconds;
        float m_forceEvalMicroseconds;
        float m_constraintEvalMicroseconds;
        float m_constraintSolveMicroseconds;

        EmptyObject *m_mouseObject;
        SpringObject *m_controlSprings[16];

    protected:
        double m_cursor_x;
        double m_cursor_y;
        atg_scs::RigidBody *m_activeBody;

        void setCursor(double x, double y);
        void moveCursor(double dx, double dy);
        void setTargetSystem(atg_scs::RigidBodySystem *system);
        void setActiveBody(atg_scs::RigidBody *body);
        void moveToLocal(double lx, double ly);
        FixedPositionConstraint *fixObject(double x, double y);
        LinkConstraint *connectObjects(atg_scs::RigidBody *target);
        BarObject *createLinkedBar(double x, double y, double density);
        DiskObject *createLinkedDisk(double r, double density);
        EmptyObject *createEmpty(EmptyObject::Style style);
        SpringObject *connectSpring(atg_scs::RigidBody *target, double x, double y);
        ConstantSpeedMotor *createMotor(atg_scs::RigidBody *base);
        SpringObject *createControlSpring(double ks, double kd);
        EmptyObject *createMouseEmpty(EmptyObject::Style style);
        void moveBefore(DemoObject *a, DemoObject *b);

        atg_scs::RigidBodySystem *m_targetSystem;

        // Interaction mode state
        InteractionMode m_interactionMode;
        std::vector<ConstraintData> m_userConstraints;
        
        // For two-click operations
        bool m_awaitingSecondClick;
        DemoObject* m_firstClickObject;
        double m_firstClickX, m_firstClickY;

        void handleModeSelection();
        void handleSelectDrag(double px, double py);
        void handleAddSpring(double px, double py);
        void handleAddFixedJoint(double px, double py);
        void handleDelete(double px, double py);
        void handleAddRod(double px, double py);

        DemoObject* findObjectAt(double px, double py);
        void createUserSpringWithBodies(atg_scs::RigidBody* body1, double lx1, double ly1,
                               atg_scs::RigidBody* body2, double lx2, double ly2,
                               atg_scs::RigidBodySystem* system);
        void createUserRodWithBodies(atg_scs::RigidBody* body1, double lx1, double ly1,
                            atg_scs::RigidBody* body2, double lx2, double ly2,
                            atg_scs::RigidBodySystem* system);
        void createUserSpring(DemoObject* obj1, double lx1, double ly1,
                            DemoObject* obj2, double lx2, double ly2);
};

#endif /* ATG_SCS_2D_DEMO_DEMO_H */