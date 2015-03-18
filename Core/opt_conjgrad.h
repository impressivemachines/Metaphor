//
//  opt_conjgrad.h
//  Metaphor
//
//  Created by SIMON WINDER on 3/12/15.
//  Copyright (c) 2015 Impressive Machines LLC. All rights reserved.
//

#ifndef Metaphor_opt_conjgrad_h
#define Metaphor_opt_conjgrad_h

namespace im
{
    // Conjugate gradient function minimizer
    
    // Derive a class from this solver and implement the function to evaluate f(x) and df(x)/dx
    // Call init with a pre-allocated vector to hold the value of x
    // Remember to initialize it to your starting point guess
    // Call step() in a while loop until it returns true, i.e while(!step());
    // You can print the progress by adding print statements in the while loop that examine fx(), delta_fx(), delta_x()
    // Get the result by calling state()
    
    enum ConjGradientUpdateMode
    {
        ConjGradientUpdateModeFletcherReeves,
        ConjGradientUpdateModePolakRibiere
    };
    
    struct ConjGradientMinParams
    {
        ConjGradientMinParams()
        {
            termination_ratio = 1e-7; // ratio of delta_fx to fx to cause termination
            gradient_ratio = 1e-8; // termination criterion for zero gradient
            bracket_max = 1.0; // bracketing operation in line minimization uses the range [0, bracket_max]
            update_mode = ConjGradientUpdateModePolakRibiere;
            iterations_max = 1000000; // number step calls before we quit
            line_min_eps = 0; // if >0 set the error tolerance for line min termination
        }
        
        double termination_ratio;
        double gradient_ratio;
        double bracket_max;
        ConjGradientUpdateMode update_mode;
        int iterations_max;
        double line_min_eps;
    };
    
    template <typename TT> class ConjGradientMin;
    
    // class used by minimizer
    template <typename TT>
    class ConjGradientLineMin : public FuncEval1D<TT>
    {
    public:
        void init(ConjGradientMin<TT> *p, int dims, TT eps)
        {
            m_p = p;
            m_vx.resize(dims);
            m_vderiv.resize(dims);
            m_eps = eps;
        }
        
        void linemin(Vec<TT> &vbase, Vec<TT> const &vdir, TT bracketmax)
        {
            m_vbase = vbase; // reference
            m_vdir = vdir; // reference
            
            TT xa, xb, xc;
            core_line_min_bracket(xa, xb, xc, this, (TT)0, bracketmax);
            core_line_min_using_derivs(m_xmin, m_fxmin, this, xa, xc, m_eps);
            
            // copy back into vbase
            core_block_blas_axpy(vbase.view(), m_vdir.view(), m_xmin);
        }
        
        TT xmin() { return m_xmin; }
        TT fxmin() { return m_fxmin; }
        
        TT eval_fx(TT x)
        {
            m_vx.copy_from(m_vbase);
            core_block_blas_axpy(m_vx.view(), m_vdir.view(), x);
            return m_p->eval_fx(m_vx);
        }
        
        TT eval_dfx(TT x)
        {
            // eval derivative
            // note that core_line_min_using_derivs always calls eval_dfx after calling eval_fx
            // and uses the same value of x, so it is not necessary to re-calculate m_vx
            m_p->eval_dfx(m_vderiv, m_vx);
            return m_vderiv.dot_product(m_vdir); // project back onto the line direction
        }
        
    private:
        ConjGradientMin<TT> *m_p;
        TT m_xmin;
        TT m_fxmin;
        Vec<TT> m_vbase;
        Vec<TT> m_vdir;
        Vec<TT> m_vx;
        Vec<TT> m_vderiv;
        TT m_eps;
    };
    
    template <typename TT>
    class ConjGradientMin
    {
    public:
        ConjGradientMin(ConjGradientMinParams const &p) : m_params(p) {}
        ConjGradientMin() {}
        virtual ~ConjGradientMin() {}
        
        // If you dont want to use the default parameters, set them before calling init()
        void set_parameters(ConjGradientMinParams const &p) { m_params = p; }
        
        // Initalize.
        // You can wrap external memory or pass in a freshly allocated vector
        void init(Vec<TT> vstate);
        
        // Take one minimization step. Call this in a loop until it returns true
        // Then get the result by calling state()
        bool step();
        
        // Returns true if the solver stopped before convergence.
        bool early_exit() const { return m_early_exit; }
        
        // Current value of the function
        TT fx() const { return m_fx; }
        
        // Get the change in function value during the last step
        TT delta_fx() const { return m_delta_fx; }
        
        // Get the distance moved during the last step (note step sizes may rise and fall)
        TT delta_x() const { return m_delta_x; }
        
        // Get/set the current state vector x
        Vec<TT> state() { return m_vstate; }
        
        // Dimenstionality of the state vector x
        int dims() const { return m_vstate.rows(); }
        
        // Returns the number of calls to step()
        int iteration_count() const { return m_iterations; }
        
        // Functions which are to be over-ridden by derived class.
        
        // Called after init is called
        virtual void eval_init() {}
        
        // Called at the start of each step.
        virtual void eval_start_step() {}
        
        // Compute the function f(X), given vector X.
        virtual TT eval_fx(Vec<TT> const &vx) = 0;
        
        // Compute the gradient vector of f(X), about X
        virtual void eval_dfx(Vec<TT> &vdfx, Vec<TT> const &vx) = 0;
        
        // Called at the end of each step. Return true if you need to early exit.
        virtual bool eval_end_step() { return false; }
        
    protected:
        bool m_early_exit;
        TT m_fx;
        TT m_delta_fx;
        TT m_delta_x;
        Vec<TT> m_vstate;
        Vec<TT> m_vdir_x;
        Vec<TT> m_vdir_g;
        Vec<TT> m_vdir_h;
        int m_iterations;
        bool m_startup;
        
        ConjGradientMinParams m_params;
        ConjGradientLineMin<TT> m_linemin;
    };
    
}


#endif